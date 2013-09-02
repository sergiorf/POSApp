#include "../Include/codebar.h"
#include "../Include/font.h"
#include "bmpfile.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/* The most commonly used set */
#define NEON	"0123456789"
#define SSET	"0123456789ABCDEF"
#define TRUE 1
#define FALSE 0
#define	BMP_DATA	200

static int ustrlen(unsigned char data[]);
static void to_upper(unsigned char source[]);
static void concat(char dest[], char source[]);
static void uconcat(unsigned char dest[], unsigned char source[]);
static void error_tag(char error_string[], int error_number);
static int dump_plot(struct zint_symbol *symbol);
static int module_is_set(struct zint_symbol *symbol, int y_coord, int x_coord);
static int is_sane(char test_string[], unsigned char source[], int length);
static void lookup(char set_string[], char *table[], char data, char dest[]);
static void ustrcpy(unsigned char target[], unsigned char source[]);
static void expand(struct zint_symbol *symbol, char data[]);
static int ctoi(char source);
static void set_module(struct zint_symbol *symbol, int y_coord, int x_coord);
static int bmp_handle(struct zint_symbol *symbol, int rotate_angle);
static int latin1_process(unsigned char source[], unsigned char preprocessed[], int *length);
static void draw_bar(char *pixelbuf, int xpos, int xlen, int ypos, int ylen, int image_width, int image_height);
static void draw_string(char *pixbuf, char input_string[], int xposn, int yposn, int smalltext, int image_width, int image_height);
static void draw_letter(char *pixelbuf, unsigned char letter, int xposn, int yposn, int smalltext, int image_width, int image_height);
static int png_to_file(struct zint_symbol *symbol, int image_height, int image_width, char *pixelbuf, int rotate_angle);
static int bmp_pixel_plot(struct zint_symbol *symbol, int image_height, int image_width, char *pixelbuf, int rotate_angle);

struct zint_render_line {
	float x, y, length, width;
	struct zint_render_line *next;      /* Pointer to next line */
};

struct zint_render_string {
	float x, y, fsize;
	float width;                        /* Suggested string width, may be 0 if none recommended */
	int length;
	unsigned char *text;
	struct zint_render_string *next;    /* Pointer to next character */
};

struct zint_render_ring {
	float x, y, radius, line_width;
	struct zint_render_ring *next;      /* Pointer to next ring */
};

struct zint_render {
	float width, height;
	struct zint_render_line *lines;	 	/* Pointer to first line */
	struct zint_render_string *strings;	/* Pointer to first string */
	struct zint_render_ring *rings;         /* Pointer to first ring */
	struct zint_render_hexagon *hexagons;   /* Pointer to first hexagon */
};

struct zint_render_hexagon {
	float x, y;
	struct zint_render_hexagon *next;   /* Pointer to next hexagon */
};

static char *C25MatrixTable[10] = {"113311", "311131", "131131", "331111", "113131", "313111",
	"133111", "111331", "311311", "131311"};

static char *C25InterTable[10] = {"11331", "31113", "13113", "33111", "11313", "31311", "13311", "11133",
	"31131", "13131"};

struct zint_symbol *ZBarcode_Create()
{
	struct zint_symbol *symbol = (struct zint_symbol*)calloc(1, sizeof(struct zint_symbol));
	
	if (!symbol) return NULL;

	symbol->symbology = BARCODE_C25MATRIX;
	strcpy(symbol->fgcolour, "000000");
	strcpy(symbol->bgcolour, "ffffff");
	strcpy(symbol->outfile, "out.png");
	symbol->scale = 1.0;
	symbol->option_1 = -1;
	symbol->option_3 = 928; // PDF_MAX
	symbol->show_hrt = 1; // Show human readable text
	return symbol;
}

void ZBarcode_Delete(struct zint_symbol *symbol)
{
	if (symbol->bitmap != NULL)
		free(symbol->bitmap);

	// If there is a rendered version, ensure it's memory is released
	if (symbol->rendered != NULL) {
		struct zint_render_line *line, *l;
		struct zint_render_string *string, *s;

		// Free lines
		line = symbol->rendered->lines;
		while(line) {
			l = line;
			line = line->next;
			free(l);
		}
		// Free Strings
		string = symbol->rendered->strings;
		while (string) {
			s = string;
			string = string->next;
			free(s->text);
			free(s);
		}

		// Free Render
		free(symbol->rendered);
	}
	free(symbol);
}

int ZBarcode_Print(struct zint_symbol *symbol, int rotate_angle)
{
	int error_number;
	char output[4];
	
	switch(rotate_angle) {
		case 0:
		case 90:
		case 180:
		case 270:
			break;
		default:
			strcpy(symbol->errtxt, "Invalid rotation angle");
			return ZERROR_INVALID_OPTION;
			break;
	}
	
	if(strlen(symbol->outfile) > 3) {
		output[0] = symbol->outfile[strlen(symbol->outfile) - 3];
		output[1] = symbol->outfile[strlen(symbol->outfile) - 2];
		output[2] = symbol->outfile[strlen(symbol->outfile) - 1];
		output[3] = '\0';
		to_upper((unsigned char*)output);

		if(!(strcmp(output, "PNG"))) {
			if(symbol->scale < 1.0) { symbol->text[0] = '\0'; }
			//error_number = png_handle(symbol, rotate_angle);
		} else if(!(strcmp(output, "BMP"))) {
			error_number = bmp_handle(symbol, rotate_angle);
		} else if(!(strcmp(output, "TXT"))) {
			error_number = dump_plot(symbol);
		} else {
			strcpy(symbol->errtxt, "Unknown output format");
			error_tag(symbol->errtxt, ZERROR_INVALID_OPTION);
			return ZERROR_INVALID_OPTION;
		}
	} else {
		strcpy(symbol->errtxt, "Unknown output format");
		error_tag(symbol->errtxt, ZERROR_INVALID_OPTION);
		return ZERROR_INVALID_OPTION;
	}

	error_tag(symbol->errtxt, error_number);
	return error_number;
}

int bmp_handle(struct zint_symbol *symbol, int rotate_angle)
{
	int textdone, main_width, comp_offset, large_bar_count;
	char textpart[10], addon[6];
	float addon_text_posn, preset_height, large_bar_height;
	int i, r, textoffset, yoffset, xoffset, latch, image_width, image_height;
	char *pixelbuf = NULL;
	int addon_latch = 0, smalltext = 0;
	int this_row, block_width, plot_height, plot_yposn, textpos;
	float row_height, row_posn;
	int error_number;
	int default_text_posn;
	int next_yposn;
	unsigned char* local_text = NULL;
	int tlen = ustrlen(symbol->text);

	if (tlen) {
		local_text = (unsigned char*)calloc(tlen + 1, sizeof(char));
		if (NULL == local_text) {
			strcpy(symbol->errtxt, "Out of memory");
			return ZERROR_MEMORY;
		}
	}

	if(symbol->show_hrt != 0 && tlen)
		latin1_process(symbol->text, local_text, &tlen);

	textdone = 0;
	main_width = symbol->width;
	strcpy(addon, "");
	comp_offset = 0;
	addon_text_posn = 0.0;
	row_height = 0;
	if(symbol->output_options & SMALL_TEXT) {
		smalltext = 1;
	}

	if (symbol->height == 0) {
		symbol->height = 50;
	}
	
	large_bar_count = 0;
	preset_height = 0.0;
	for(i = 0; i < symbol->rows; i++) {
		preset_height += symbol->row_height[i];
		if(symbol->row_height[i] == 0) {
			large_bar_count++;
		}
	}

	if (large_bar_count == 0) {
		symbol->height = preset_height;
		large_bar_height = 10;
	} else {
		large_bar_height = (symbol->height - preset_height) / large_bar_count;
	}
	
	while(!(module_is_set(symbol, symbol->rows - 1, comp_offset))) {
		comp_offset++;
	}
	
	latch = 0;
	r = 0;
	/* No add-on text */
	addon[r] = '\0';
	
	if(tlen) {
		textoffset = 9;
	} else {
		textoffset = 0;
	}
	xoffset = symbol->border_width + symbol->whitespace_width;
	yoffset = symbol->border_width;
	image_width = 2 * (symbol->width + xoffset + xoffset);
	image_height = 2 * (symbol->height + textoffset + yoffset + yoffset);
	
	if (!(pixelbuf = (char *) malloc(image_width * image_height))) {
		printf("Insufficient memory for pixel buffer");
		if(local_text)
			free(local_text);

		return ZERROR_ENCODING_PROBLEM;
	} else {
		for(i = 0; i < (image_width * image_height); i++) {
			*(pixelbuf + i) = '0';
		}
	}
	
	default_text_posn = image_height - 17 - symbol->border_width - symbol->border_width;
	row_posn = textoffset + yoffset;
	next_yposn = textoffset + yoffset;
	row_height = 0;

	/* Plot the body of the symbol to the pixel buffer */
	for(r = 0; r < symbol->rows; r++) {
		this_row = symbol->rows - r - 1; /* invert r otherwise plots upside down */
		row_posn += row_height;
		plot_yposn = next_yposn;
		if(symbol->row_height[this_row] == 0) {
			row_height = large_bar_height;
		} else {
			row_height = symbol->row_height[this_row];
		}
		next_yposn = (int)(row_posn + row_height);
		plot_height = next_yposn - plot_yposn;
		
		i = 0;
		if(module_is_set(symbol, this_row, 0)) {
			latch = 1;
		} else {
			latch = 0;
		}
			
		do {
			block_width = 0;
			do {
				block_width++;
			} while (module_is_set(symbol, this_row, i + block_width) == module_is_set(symbol, this_row, i));
			if((addon_latch == 0) && (r == 0) && (i > main_width)) {
				plot_height = (int)(row_height - 5.0);
				plot_yposn = (int)(row_posn - 5.0);
				addon_text_posn = row_posn + row_height - 8.0;
				addon_latch = 1;
			} 
			if(latch == 1) { 
				/* a bar */
				draw_bar(pixelbuf, (i + xoffset) * 2, block_width * 2, plot_yposn * 2, plot_height * 2, image_width, image_height);
				latch = 0;
			} else {
				/* a space */
				latch = 1;
			}
			i += block_width;
				
		} while (i < symbol->width);
	}
	
	xoffset += comp_offset;
	xoffset -= comp_offset;
	
	/* Put the human readable text at the bottom */
	if((textdone == 0) && tlen) {
		textpos = (image_width / 2);
		draw_string(pixelbuf, (char*)local_text, textpos, default_text_posn, smalltext, image_width, image_height);
	}

	error_number=png_to_file(symbol, image_height, image_width, pixelbuf, rotate_angle);
	free(pixelbuf);
	
	if (local_text)
		free(local_text);

	return error_number;
}

int interleaved_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length)
{ /* Code 2 of 5 Interleaved */

	int i, j, k, error_number;
	char bars[7], spaces[7], mixed[14], dest[1000];
	unsigned char* temp = (unsigned char *)malloc((length + 2) * sizeof(unsigned char));
	error_number = 0;
	
	if(length > 89) {
		strcpy(symbol->errtxt, "Input too long");
		return ZERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if (error_number == ZERROR_INVALID_DATA) {
		strcpy(symbol->errtxt, "Invalid characters in data");
		return error_number;
	}
	
	ustrcpy(temp, (unsigned char *) "");
	/* Input must be an even number of characters for Interlaced 2 of 5 to work:
	   if an odd number of characters has been entered then add a leading zero */
	if (length & 1)
	{
		ustrcpy(temp, (unsigned char *) "0");
		length++;
	}
	uconcat(temp, source);

	/* start character */
	strcpy(dest, "1111");

	for(i = 0; i < length; i+=2 )
	{
		/* look up the bars and the spaces and put them in two strings */
		strcpy(bars, "");
		lookup(NEON, C25InterTable, temp[i], bars);
		strcpy(spaces, "");
		lookup(NEON, C25InterTable, temp[i + 1], spaces);

		/* then merge (interlace) the strings together */
		k = 0;
		for(j = 0; j <= 4; j++)
		{
			mixed[k] = bars[j]; k++;
			mixed[k] = spaces[j]; k++;
		}
		mixed[k] = '\0';
		concat (dest, mixed);
	}

	/* Stop character */
	concat (dest, "311");
	
	expand(symbol, dest);
	ustrcpy(symbol->text, temp);
	return error_number;
}

int matrix_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length)
{ /* Code 2 of 5 Standard (Code 2 of 5 Matrix) */
	
	int i, error_number;
	char dest[512]; /* 6 + 80 * 6 + 6 + 1 ~ 512*/

	error_number = 0;
	
	if(length > 80) {
		strcpy(symbol->errtxt, "Input too long");
		return ZERROR_TOO_LONG;
	}
	error_number = is_sane(NEON, source, length);
	if(error_number == ZERROR_INVALID_DATA) {
		strcpy(symbol->errtxt, "Invalid characters in data");
		return error_number;
	}
	
	/* start character */
	strcpy(dest, "411111");

	for(i = 0; i < length; i++) {
		lookup(NEON, C25MatrixTable, source[i], dest);
	}

	/* Stop character */
	concat (dest, "41111");
	
	expand(symbol, dest);
	ustrcpy(symbol->text, source);
	return error_number;
}

void to_upper(unsigned char source[])
{ /* Converts lower case characters to upper case in a string source[] */
	unsigned int i, src_len = ustrlen(source);

	for (i = 0; i < src_len; i++) {
		if ((source[i] >= 'a') && (source[i] <= 'z')) {
			source [i] = (source[i] - 'a') + 'A'; }
	}
}

void uconcat(unsigned char dest[], unsigned char source[])
{ /* Concatinates dest[] with the contents of source[], copying /0 as well */
	unsigned int i, j;

	j = ustrlen(dest);
	for(i = 0; i <= ustrlen(source); i++) {
		dest[i + j] = source[i]; }
}

void error_tag(char error_string[], int error_number)
{
	char error_buffer[100];
	
	if(error_number != 0) {
		strcpy(error_buffer, error_string);
		
		if(error_number > 4) {
			strcpy(error_string, "error: ");
		} else {
			strcpy(error_string, "warning: ");
		}
		
		concat(error_string, error_buffer);
	}
}

void concat(char dest[], char source[])
{ /* Concatinates dest[] with the contents of source[], copying /0 as well */
	unsigned int i, j, n;

	j = strlen(dest);
	n = strlen(source);
	for(i = 0; i <= n; i++) {
		dest[i + j] = source[i]; }
}

int ustrlen(unsigned char data[]) {
	/* Local replacement for strlen() with unsigned char strings */
	int i;
	for (i=0;data[i];i++);

	return i;
}

int dump_plot(struct zint_symbol *symbol)
{
	FILE *f;
	int i, r;
	f = fopen(symbol->outfile, "w");
	if(!f) {
		strcpy(symbol->errtxt, "Could not open output file");
		return ZERROR_FILE_ACCESS;
	}
	fputs("[\n", f);
	for (r = 0; r < symbol->rows; r++) {
		fputs(" [ ", f);
		for (i = 0; i < symbol->width; i++) {
			fputs(module_is_set(symbol, r, i) ? "1 " : "0 ", f);
		}
		fputs("]\n", f);
	}
	fputs("]\n", f);
	fclose(f);
	return 0;
}

int module_is_set(struct zint_symbol *symbol, int y_coord, int x_coord)
{
	return (symbol->encoded_data[y_coord][x_coord / 7] >> (x_coord % 7)) & 1;
#if 0	
	switch(x_sub) {
		case 0: if((symbol->encoded_data[y_coord][x_char] & 0x01) != 0) { result = 1; } break;
		case 1: if((symbol->encoded_data[y_coord][x_char] & 0x02) != 0) { result = 1; } break;
		case 2: if((symbol->encoded_data[y_coord][x_char] & 0x04) != 0) { result = 1; } break;
		case 3: if((symbol->encoded_data[y_coord][x_char] & 0x08) != 0) { result = 1; } break;
		case 4: if((symbol->encoded_data[y_coord][x_char] & 0x10) != 0) { result = 1; } break;
		case 5: if((symbol->encoded_data[y_coord][x_char] & 0x20) != 0) { result = 1; } break;
		case 6: if((symbol->encoded_data[y_coord][x_char] & 0x40) != 0) { result = 1; } break;
	}
	
	return result;
#endif
}

int is_sane(char test_string[], unsigned char source[], int length)
{ /* Verifies that a string only uses valid characters */
	unsigned int i, j, latch;
	unsigned int lt = strlen(test_string);

	for(i = 0; i < length; i++) {
		latch = FALSE;
		for(j = 0; j < lt; j++) {
			if (source[i] == test_string[j]) { 
				latch = TRUE; 
				break;
			} 
		}
		if (!(latch)) { 
			return ZERROR_INVALID_DATA; 
		}
	}
	
	return 0;
}

void lookup(char set_string[], char *table[], char data, char dest[])
{ /* Replaces huge switch statements for looking up in tables */
	unsigned int i, n = strlen(set_string);

	for(i = 0; i < n; i++) {
		if (data == set_string[i]) { concat(dest, table[i]); } }
}

void ustrcpy(unsigned char target[], unsigned char source[]) {
	/* Local replacement for strcpy() with unsigned char strings */
	int i, len;

	len = ustrlen(source);
	for(i = 0; i < len; i++) {
		target[i] = source[i];
	}
	target[i] = '\0';
}


void expand(struct zint_symbol *symbol, char data[])
{ /* Expands from a width pattern to a bit pattern */
	
	unsigned int reader, n = strlen(data);
	int writer, i;
	char latch;
	
	writer = 0;
	latch = '1';
	
	for(reader = 0; reader < n; reader++) {
		for(i = 0; i < ctoi(data[reader]); i++) {
			if(latch == '1') { set_module(symbol, symbol->rows, writer); }
			writer++;
		}

		latch = (latch == '1' ? '0' : '1');
	}
	
		if(writer > symbol->width) {
			symbol->width = writer;
		}
	symbol->rows = symbol->rows + 1;
}

int ctoi(char source)
{ /* Converts a character 0-9 to its equivalent integer value */
	if((source >= '0') && (source <= '9'))
		return (source - '0');
	return(source - 'A' + 10);
}

void set_module(struct zint_symbol *symbol, int y_coord, int x_coord)
{
	symbol->encoded_data[y_coord][x_coord / 7] |= 1 << (x_coord % 7);
#if 0
	int x_char, x_sub;
	

	x_char = x_coord / 7;
	x_sub = x_coord % 7;
	
	switch(x_sub) {
		case 0: symbol->encoded_data[y_coord][x_char] += 0x01; break;
		case 1: symbol->encoded_data[y_coord][x_char] += 0x02; break;
		case 2: symbol->encoded_data[y_coord][x_char] += 0x04; break;
		case 3: symbol->encoded_data[y_coord][x_char] += 0x08; break;
		case 4: symbol->encoded_data[y_coord][x_char] += 0x10; break;
		case 5: symbol->encoded_data[y_coord][x_char] += 0x20; break;
		case 6: symbol->encoded_data[y_coord][x_char] += 0x40; break;
	} /* The last binary digit is reserved for colour barcodes */
#endif
}

int latin1_process(unsigned char source[], unsigned char preprocessed[], int *length)
{
	int j, i, next;
	
	/* Convert Unicode to Latin-1 for those symbologies which only support Latin-1 */
	j = 0;
	i = 0;
	if (length && *length) {
		do {
			next = -1;
			if(source[i] < 128) {
				preprocessed[j] = source[i];
				j++;
				next = i + 1;
			} else {
				if(source[i] == 0xC2) {
					preprocessed[j] = source[i + 1];
					j++;
					next = i + 2;
				}
				if(source[i] == 0xC3) {
					preprocessed[j] = source[i + 1] + 64;
					j++;
					next = i + 2;
				}
			}
			if(next == -1) {				
				return ZERROR_INVALID_DATA;
			}
			i = next;
		} while(i < *length);
		preprocessed[j] = '\0';
		*length = j;
	}

	return 0;
}

void draw_bar(char *pixelbuf, int xpos, int xlen, int ypos, int ylen, int image_width, int image_height)
{
	/* Draw a rectangle */
	int i, j, png_ypos;
	
	png_ypos = image_height - ypos - ylen;
	/* This fudge is needed because EPS measures height from the bottom up but
	PNG measures y position from the top down */
	
	for(i = (xpos); i < (xpos + xlen); i++) {
		for( j = (png_ypos); j < (png_ypos + ylen); j++) {
			*(pixelbuf + (image_width * j) + i) = '1';
		}
	}
}

void draw_string(char *pixbuf, char input_string[], int xposn, int yposn, int smalltext, int image_width, int image_height)
{
	/* Plot a string into the pixel buffer */
	int i, string_length, string_left_hand;
	
	string_length = strlen(input_string);
	string_left_hand = xposn - ((7 * string_length) / 2);
	
	for(i = 0; i < string_length; i++) {
		draw_letter(pixbuf, input_string[i], string_left_hand + (i * 7), yposn, smalltext, image_width, image_height);
	}
}

void draw_letter(char *pixelbuf, unsigned char letter, int xposn, int yposn, int smalltext, int image_width, int image_height)
{
	/* Put a letter into a position */
	int skip, i, j, glyph_no, alphabet;
	
	skip = 0;
	alphabet = 0;

	image_height = 0; /* to silence compiler warning */
	
	if(letter < 33) { skip = 1; }
	if((letter > 127) && (letter < 161)) { skip = 1; }
	
	if(skip == 0) {
		if(letter > 128) {
			alphabet = 1;
			glyph_no = letter - 161;
		} else {
			glyph_no = letter - 33;
		}
		
		if(smalltext) {
			for(i = 0; i <= 8; i++) {
				for(j = 0; j < 5; j++) {
					if(alphabet == 0) {
						if(small_font[(glyph_no * 5) + (i * 475) + j - 1] == 1) {
							*(pixelbuf + (i * image_width) + (yposn * image_width) + xposn + j) = '1';
						}
					} else {
						if(small_font_extended[(glyph_no * 5) + (i * 475) + j - 1] == 1) {
							*(pixelbuf + (i * image_width) + (yposn * image_width) + xposn + j) = '1';
						}
					}
				}
			}
		} else {
			for(i = 0; i <= 13; i++) {
				for(j = 0; j < 7; j++) {
					if(alphabet == 0) {
						if(ascii_font[(glyph_no * 7) + (i * 665) + j - 1] == 1) {
							*(pixelbuf + (i * image_width) + (yposn * image_width) + xposn + j) = '1';
						}
					} else {
						if(ascii_ext_font[(glyph_no * 7) + (i * 665) + j - 1] == 1) {
							*(pixelbuf + (i * image_width) + (yposn * image_width) + xposn + j) = '1';
						}
					}
				}
			}
		}
	}
}

int png_to_file(struct zint_symbol *symbol, int image_height, int image_width, char *pixelbuf, int rotate_angle)
{
	int error_number;
	float scaler = symbol->scale;
	char *scaled_pixelbuf;
	int horiz, vert;
	int scale_width, scale_height;
	
	if(scaler == 0) { scaler = 0.5; }
	scale_width = image_width * scaler;
	scale_height = image_height * scaler;
	
	/* Apply scale options by creating another pixel buffer */
	if ((scaled_pixelbuf = (char *) malloc(scale_width * scale_height)) == NULL) {
		printf("Insufficient memory for pixel buffer");
		return ZERROR_ENCODING_PROBLEM;
	}
	memset(scaled_pixelbuf, '0', scale_width * scale_height);
	
	for(vert = 0; vert < scale_height; vert++) {
		for(horiz = 0; horiz < scale_width; horiz++) {
			*(scaled_pixelbuf + (vert * scale_width) + horiz) = *(pixelbuf + ((int)(vert / scaler) * image_width) + (int)(horiz / scaler));
		}
	}
	error_number = bmp_pixel_plot(symbol, scale_height, scale_width, scaled_pixelbuf, rotate_angle);
	if (0==error_number) {
		bmpfile_t* bmp = bmp_create(symbol->bitmap_width,symbol->bitmap_height,1);
		if (NULL!=bmp) {
			int y,x;
			int i=0;
			for(y = 0; y < symbol->bitmap_height; y++) {
				for(x = 0; x < symbol->bitmap_width; x++) {
					bw_pixel_t pixel;
					pixel.uniColour = symbol->bitmap[i++];
					bmp_set_pixel(bmp,x,y,pixel);
				}
			}
			bmp_save(bmp, symbol->outfile);
			bmp_destroy(bmp);
		}
	}
	free(scaled_pixelbuf);
	return error_number;
}

int bmp_pixel_plot(struct zint_symbol *symbol, int image_height, int image_width, char *pixelbuf, int rotate_angle)
{
	unsigned long rowbytes;
	int i, row, column, err_no;
	int fgred, fggrn, fgblu, bgred, bggrn, bgblu;
	
	switch(rotate_angle) {
		case 0:
		case 180:
			symbol->bitmap_width = image_width;
			symbol->bitmap_height = image_height;
			break;
		case 90:
		case 270:			
			symbol->bitmap_width = image_height;
			symbol->bitmap_height = image_width;
			break;
	}
	
	if (symbol->bitmap != NULL)
		free(symbol->bitmap);

    symbol->bitmap = (char *) malloc(image_width * image_height);

	
	/* sort out colour options */
	to_upper((unsigned char*)symbol->fgcolour);
	to_upper((unsigned char*)symbol->bgcolour);
	
	if(strlen(symbol->fgcolour) != 6) {
		strcpy(symbol->errtxt, "Malformed foreground colour target");
		return ZERROR_INVALID_OPTION;
	}
	if(strlen(symbol->bgcolour) != 6) {
		strcpy(symbol->errtxt, "Malformed background colour target");
		return ZERROR_INVALID_OPTION;
	}
	err_no = is_sane(SSET, (unsigned char*)symbol->fgcolour, strlen(symbol->fgcolour));
	if (err_no == ZERROR_INVALID_DATA) {
		strcpy(symbol->errtxt, "Malformed foreground colour target");
		return ZERROR_INVALID_OPTION;
	}
	err_no = is_sane(SSET, (unsigned char*)symbol->bgcolour, strlen(symbol->fgcolour));
	if (err_no == ZERROR_INVALID_DATA) {
		strcpy(symbol->errtxt, "Malformed background colour target");
		return ZERROR_INVALID_OPTION;
	}
	
	fgred = (16 * ctoi(symbol->fgcolour[0])) + ctoi(symbol->fgcolour[1]);
	fggrn = (16 * ctoi(symbol->fgcolour[2])) + ctoi(symbol->fgcolour[3]);
	fgblu = (16 * ctoi(symbol->fgcolour[4])) + ctoi(symbol->fgcolour[5]);
	bgred = (16 * ctoi(symbol->bgcolour[0])) + ctoi(symbol->bgcolour[1]);
	bggrn = (16 * ctoi(symbol->bgcolour[2])) + ctoi(symbol->bgcolour[3]);
	bgblu = (16 * ctoi(symbol->bgcolour[4])) + ctoi(symbol->bgcolour[5]);

	/* set rowbytes - depends on picture depth */
	rowbytes = symbol->bitmap_width;

	/* Pixel Plotting */
	i = 0;	
	switch(rotate_angle) {
		case 0: /* Plot the right way up */
			for(row = 0; row < image_height; row++) {
				for(column = 0; column < image_width; column++) {
					switch(*(pixelbuf + (image_width * row) + column))
					{
						case '1':
							symbol->bitmap[i++] = fgred;
							/*symbol->bitmap[i++] = fggrn;
							symbol->bitmap[i++] = fgblu;
							*/
							break;
						default:
							symbol->bitmap[i++] = bgred;
							/*symbol->bitmap[i++] = bggrn;
							symbol->bitmap[i++] = bgblu;
							*/
							break;
				
					}
				}
			}
			break;
		case 90: /* Plot 90 degrees clockwise */			
			for(row = 0; row < image_width; row++) {
				for(column = 0; column < image_height; column++) {
					switch(*(pixelbuf + (image_width * (image_height - column - 1)) + row))
					{
						case '1':
							symbol->bitmap[i++] = fgred;
							/*symbol->bitmap[i++] = fggrn;
							symbol->bitmap[i++] = fgblu;
							*/
							break;
						default:
							symbol->bitmap[i++] = bgred;
							/*symbol->bitmap[i++] = bggrn;
							symbol->bitmap[i++] = bgblu;
							*/
							break;
			
					}
				}
			}
			break;
		case 180: /* Plot upside down */
			for(row = 0; row < image_height; row++) {
				for(column = 0; column < image_width; column++) {
					switch(*(pixelbuf + (image_width * (image_height - row - 1)) + (image_width - column - 1)))
					{
						case '1':
							symbol->bitmap[i++] = fgred;
							/*symbol->bitmap[i++] = fggrn;
							symbol->bitmap[i++] = fgblu;
							*/
							break;
						default:
							symbol->bitmap[i++] = bgred;
							/*symbol->bitmap[i++] = bggrn;
							symbol->bitmap[i++] = bgblu;
							*/
							break;
			
					}
				}
			}
			break;
		case 270: /* Plot 90 degrees anti-clockwise */
			for(row = 0; row < image_width; row++) {
				for(column = 0; column < image_height; column++) {
					switch(*(pixelbuf + (image_width * column) + (image_width - row - 1)))
					{
						case '1':
							symbol->bitmap[i++] = fgred;
							/*symbol->bitmap[i++] = fggrn;
							symbol->bitmap[i++] = fgblu;
							*/
							break;
						default:
							symbol->bitmap[i++] = bgred;
							/*symbol->bitmap[i++] = bggrn;
							symbol->bitmap[i++] = bgblu;
							*/
							break;
	
					}
				}
			}
			break;
	}

	return 0;
}


int ZBarcode_gentofile(const char* code, const char* filename)
{
	int error_number = 0;
    struct zint_symbol *symbol;
   
    symbol = ZBarcode_Create();
    symbol->input_mode = UNICODE_MODE;
    symbol->symbology = BARCODE_C25INTER; 
    symbol->height = 0;
    symbol->whitespace_width = 0;
    symbol->border_width = 0;
    symbol->output_options = 0;
    strncpy(symbol->outfile, filename, strlen(filename));
    symbol->scale = 1;
    symbol->option_1 = 0xffffffff;
    symbol->option_2 = 0;
    symbol->option_3 = 0x000003a0;
    symbol->show_hrt = 1;

    symbol->rows = 0;
    symbol->width = 0;
    symbol->bitmap_width = 0;
    symbol->bitmap_height = 0;
    strncpy(symbol->fgcolour, "000000", 10);
    strncpy(symbol->bgcolour, "ffffff", 10);
    
	error_number = interleaved_two_of_five
		(symbol, (unsigned char*)code, strlen(code));
    if(error_number == 0) {
		error_number = ZBarcode_Print(symbol, 270);
		/*WRITE_AT("done ...",3,3);
		SVC_WAIT(3000);
		*/
		    }    
	if (NULL!=symbol)
		ZBarcode_Delete(symbol);
	return error_number;
}

#ifdef _CUTEST

void test_codebarCreate(CuTest* tc)
{
	int error_number = 0;
    struct zint_symbol *symbol;
    unsigned char code[] = { "0114086593308"};
   
    symbol = ZBarcode_Create();
    symbol->input_mode = UNICODE_MODE;
    symbol->symbology = BARCODE_C25INTER; 
    symbol->height = 0;
    symbol->whitespace_width = 0;
    symbol->border_width = 0;
    symbol->output_options = 0;
    strncpy(symbol->outfile, "test.bmp", 10);
    symbol->scale = 1.0;
    symbol->option_1 = 0xffffffff;
    symbol->option_2 = 0;
    symbol->option_3 = 0x000003a0;
    symbol->show_hrt = 1;

    symbol->rows = 0;
    symbol->width = 0;
    symbol->bitmap_width = 0;
    symbol->bitmap_height = 0;
    strncpy(symbol->fgcolour, "000000", 10);
    strncpy(symbol->bgcolour, "ffffff", 10);
    
	//error_number = matrix_two_of_five(symbol, code, 9);
	error_number = interleaved_two_of_five(symbol, code, 13);
    if(error_number == 0) {
        error_number = ZBarcode_Print(symbol, 0);
    }    
    if(error_number != 0) {
        printf("%s\n", symbol->errtxt);
        ZBarcode_Delete(symbol);
    } 
	CuAssertTrue(tc,0==error_number);
}

CuSuite* CuGetCodebarSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_codebarCreate);
    return suite;
}

#endif








