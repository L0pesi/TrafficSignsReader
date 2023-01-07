#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Alocar memória para uma imagem
IVC* vc_image_new(int width, int height, int channels, int levels)
{
	IVC* image = (IVC*)malloc(sizeof(IVC));

	if (image == NULL)
		return NULL;
	if ((levels <= 0) || (levels > 255))
		return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char*)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

// Libertar memória de uma imagem
IVC* vc_image_free(IVC* image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char* netpbm_get_token(FILE* file, char* tok, int len)
{
	char* t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)))
			;
		if (c != '#')
			break;
		do
			c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF)
			break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#')
			ungetc(c, file);
	}

	*t = 0;

	return tok;
}

long int unsigned_char_to_bit(unsigned char* datauchar, unsigned char* databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char* p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char* databit, unsigned char* datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char* p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}

IVC* vc_read_image(char* filename)
{
	FILE* file = NULL;
	IVC* image = NULL;
	unsigned char* tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0)
		{
			channels = 1;
			levels = 1;
		} // Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0)
			channels = 1; // Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0)
			channels = 3; // Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}

int vc_write_image(char* filename, IVC* image)
{
	FILE* file = NULL;
	unsigned char* tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL)
		return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char*)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

int vc_gray_negative(IVC* srcdst)
{
	if (srcdst != NULL)
	{
		int x, y;
		long int pos;

		// Validação de erros
		if (srcdst->channels != 1 || srcdst->width <= 0 || srcdst->height <= 0 || srcdst->data == NULL)
			return 0;

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				//calcular a posição do byte na posição do pixel
				pos = y * srcdst->bytesperline + x * srcdst->channels;

				srcdst->data[pos] = 255 - srcdst->data[pos];
			}
		}
	}

	return 1;
}

int vc_rgb_negative(IVC* srcdst)
{
	if (srcdst != NULL)
	{
		int x, y, z;
		long int pos;

		// Validação de erros
		if (srcdst->channels != 3 || srcdst->width <= 0 || srcdst->height <= 0 || srcdst->data == NULL)
			return 0;

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				pos = y * srcdst->bytesperline + x * srcdst->channels;

				srcdst->data[pos] = 255 - srcdst->data[pos];
				srcdst->data[pos + 1] = 255 - srcdst->data[pos + 1];
				srcdst->data[pos + 2] = 255 - srcdst->data[pos + 2];
			}
		}
	}

	return 1;
}

int vc_rgb_get_red_gray(IVC* srcdst)
{
	if (srcdst != NULL)
	{
		int x, y, z;
		long int pos;

		// Validação de erros
		if (srcdst->channels != 3 || srcdst->width <= 0 || srcdst->height <= 0 || srcdst->data == NULL)
			return 0;

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				pos = y * srcdst->bytesperline + x * srcdst->channels;
				//se fosse obter apenas o vermelho basta apenas colocar a 0 o valor do verde e azul
				srcdst->data[pos + 1] = srcdst->data[pos]; //verde
				srcdst->data[pos + 2] = srcdst->data[pos]; //azul
			}
		}
	}

	return 1;
}

int vc_rgb_get_green_gray(IVC* srcdst)
{
	if (srcdst != NULL)
	{
		int x, y, z;
		long int pos;

		// Validação de erros
		if (srcdst->channels != 3 || srcdst->width <= 0 || srcdst->height <= 0 || srcdst->data == NULL)
			return 0;

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				pos = y * srcdst->bytesperline + x * srcdst->channels;
				//se fosse obter apenas o verde basta apenas colocar a 0 o valor do vermelho e azul
				srcdst->data[pos] = srcdst->data[pos + 1];	   //vermelho
				srcdst->data[pos + 2] = srcdst->data[pos + 1]; //azul
			}
		}
	}

	return 1;
}

int vc_rgb_get_blue_gray(IVC* srcdst)
{
	if (srcdst != NULL)
	{
		int x, y, z;
		long int pos;

		// Validação de erros
		if (srcdst->channels != 3 || srcdst->width <= 0 || srcdst->height <= 0 || srcdst->data == NULL)
			return 0;

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				pos = y * srcdst->bytesperline + x * srcdst->channels;
				//se fosse obter apenas o verde basta apenas colocar a 0 o valor do vermelho e azul
				srcdst->data[pos] = srcdst->data[pos + 2];	   //vermelho
				srcdst->data[pos + 1] = srcdst->data[pos + 2]; //verde
			}
		}
	}

	return 1;
}

int vc_rgb_to_gray(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->bytesperline;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->bytesperline;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	//verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}
	return 1;
}

// Conversão de RGB para HSV
int vc_rgb_to_hsv(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores máximo e mínimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else /* rgb_max == b*/
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		// Atribui valores entre [0,255]
		data[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);
	}

	return 1;
}

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC* srcdst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int hue, saturation, value;
	int h, s, v; // h=[0, 360] s=[0, 100] v=[0, 100]
	int i, size;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		h = ((float)data[i]) / 255.0f * 360.0f;
		s = ((float)data[i + 1]) / 255.0f * 100.0f;
		v = ((float)data[i + 2]) / 255.0f * 100.0f;

		if ((h > hmin) && (h <= hmax) && (s >= smin) && (s <= smax) && (v >= vmin) && (v <= vmax))
		{
			data[i] = 255;
			data[i + 1] = 255;
			data[i + 2] = 255;
		}
		else
		{
			data[i] = 0;
			data[i + 1] = 0;
			data[i + 2] = 0;
		}
	}

	return 1;
}

int vc_scale_gray_to_rgb(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int width = src->width;
	int height = src->height;
	int d, i;
	long int pos_src, pos_dst;
	unsigned char red[255], green[255], blue[255], brilho;

	for (d = 0, i = 0; i < 64; i++, d += 4)
	{
		red[i] = 0;
		green[i] = d;
		blue[i] = 255;
	}

	for (d = 255, i = 64; i < 128; i++, d -= 4)
	{
		red[i] = 0;
		green[i] = 255;
		blue[i] = d;
	}

	for (d = 0, i = 128; i < 192; i++, d += 4)
	{
		red[i] = d;
		green[i] = 255;
		blue[i] = 0;
	}

	for (d = 255, i = 192; i < 255; i++, d -= 4)
	{
		red[i] = 255;
		green[i] = d;
		blue[i] = 0;
	}

	for (pos_src = 0, pos_dst = 0; pos_src < bytesperline_src * height; pos_src++, pos_dst += 3)
	{
		brilho = datasrc[pos_src];
		datadst[pos_dst] = red[brilho];
		datadst[pos_dst + 1] = green[brilho];
		datadst[pos_dst + 2] = blue[brilho];
	}
	return 1;
}

int vc_gray_to_binary(IVC* srcdst, int threshold)
{
	if (srcdst != NULL)
	{
		int x, y;
		long int pos;

		// Validação de erros
		if (srcdst->channels != 1 || srcdst->width <= 0 || srcdst->height <= 0 || srcdst->data == NULL)
			return 0;

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				pos = y * srcdst->bytesperline + x * srcdst->channels;

				if (srcdst->data[pos] > threshold)
					srcdst->data[pos] = 255;
				else
					srcdst->data[pos] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_global_mean(IVC* srcdst)
{
	if (srcdst != NULL)
	{
		int x, y;
		long int pos, sum = 0, average = 0;

		// Validação de erros
		if (srcdst->channels != 1 || srcdst->width <= 0 || srcdst->height <= 0 || srcdst->data == NULL)
			return 0;

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				pos = y * srcdst->bytesperline + x * srcdst->channels;
				sum += srcdst->data[pos];
			}
		}

		average = sum / (srcdst->width * srcdst->height);

		for (x = 0; x < srcdst->width; x++)
		{
			for (y = 0; y < srcdst->bytesperline + x * srcdst->channels; y++)
			{
				pos = y * srcdst->bytesperline + x * srcdst->channels;
				if (srcdst->data[pos] > average)
					srcdst->data[pos] = 255;
				else
					srcdst->data[pos] = 0;
			}
		}
	}

	return 1;
}

// Converter de Gray para Binário (threshold automático Midpoint)
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2; //(int) floor(((double) kernel) / 2.0);
	int max, min;
	long int pos, posk;
	unsigned char threshold;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			max = 0;
			min = 255;

			// NxM Vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						//cálculo do mínimo e do máximo a estudar na vizinhança
						if (datasrc[posk] > max)
							max = datasrc[posk];
						if (datasrc[posk] < min)
							min = datasrc[posk];
					}
				}
			}

			//Cálculo do midpoint
			threshold = (unsigned char)((float)(max + min) / (float)2);

			if (datasrc[pos] > threshold)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}

	return 1;
}

// Converter de Gray para Binário (threshold automático Midpoint)
int vc_gray_to_binary_bernson(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2; //(int) floor(((double) kernel) / 2.0);
	int max, min;
	long int pos, posk;
	unsigned char threshold;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			max = 0;
			min = 255;

			// NxM Vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						//cálculo do mínimo e do máximo a estudar na vizinhança
						if (datasrc[posk] > max)
							max = datasrc[posk];
						if (datasrc[posk] < min)
							min = datasrc[posk];
					}
				}
			}

			//Cálculo do midpoint
			threshold = (unsigned char)((float)(max + min) / (float)2);

			if (datasrc[pos] > threshold)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}

	return 1;
}

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	long int pos;
	int y, x;
	int aux;

	int offset = kernel / 2;
	int ky, kx;
	long int posk;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels_src != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;
			aux = 0;

			// NxM Vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline_src + (x + kx) * channels_src;

						if (datasrc[posk] == 255)
						{
							aux = 255;
						}
					}
				}
			}

			if (aux == 255)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}
	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	long int pos;
	int y, x;
	int nb;

	int offset = kernel / 2;
	int ky, kx;
	long int posk;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels_src != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;
			nb = 255;

			// NxM Vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline_src + (x + kx) * channels_src;

						if (datasrc[posk] == 0)
						{
							nb = 0;
						}
					}
				}
			}

			if (nb != 0)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}
	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernel)
{
	int ret = 1;

	IVC* aux = vc_image_new(src->width, src->height, src->channels, src->levels);

	ret &= vc_binary_erode(src, aux, kernel);
	ret &= vc_binary_dilate(aux, dst, kernel);

	vc_image_free(aux);

	return ret;
}

int vc_binary_close(IVC* src, IVC* dst, int kernel)
{
	int ret = 1;

	IVC* aux = vc_image_new(src->width, src->height, src->channels, src->levels);

	ret &= vc_binary_dilate(src, aux, kernel);
	ret &= vc_binary_erode(aux, dst, kernel);

	vc_image_free(aux);

	return ret;
}

// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return NULL;
	if (channels != 1)
		return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 255 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0)
			datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels;		// B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels;		// D
			posX = y * bytesperline + x * channels;				// X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0)
						num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num))
						num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num))
						num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num))
						num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b])
				labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++;						  // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0)
		return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++)
			blobs[a].label = labeltable[a];
	}
	else
		return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x)
						xmin = x;
					if (ymin > y)
						ymin = y;
					if (xmax < x)
						xmax = x;
					if (ymax < y)
						ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_gray_histogram_show(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	int contarpixeis[256] = { 0 };
	float pdf[256];
	float conta = 0;
	float max = 0;
	double temp;
	float cdf[256] = { 0 };
	float equalizacao[256] = { 0 };

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	//numero pixeis repetidos
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * byteperline + x * channels;
			contarpixeis[(int)data[pos]]++;
		}
	}

	//calcula pdf
	for (y = 0; y < 256; y++)
	{
		pdf[y] = (float)contarpixeis[y] / (float)(width * height);
		conta += pdf[y];

		if (max < pdf[y])
			max = pdf[y];
	}

	//calcula grafico cdf
	for (x = 0; x < 256; x++)
	{
		for (y = x; y >= 0; y--)
		{
			cdf[x] += pdf[y];
		}
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * byteperline + x * channels;
			dst->data[pos] = cdf[data[pos]] * 255;
		}
	}
	return 1;
}

int vc_histograma_equalization(IVC* src, IVC* dst)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	int contarpixeis[256] = { 0 };
	float pdf[256];
	float conta = 0;
	float max = 0;
	double temp;
	float cdf[256] = { 0 };
	float equalizacao[256] = { 0 };

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	//numero pixeis repetidos
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * byteperline + x * channels;
			contarpixeis[(int)data[pos]]++;
		}
	}

	//calcula pdf
	for (y = 0; y < 256; y++)
	{
		pdf[y] = (float)contarpixeis[y] / (float)(width * height);
		conta += pdf[y];

		if (max < pdf[y])
			max = pdf[y];
	}

	//calcula grafico cdf
	for (int i = 1; i < 256; i++)
	{
		cdf[i] = cdf[i - 1] + pdf[i];
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * byteperline + x * channels;
			dst->data[pos] = cdf[data[pos]] * src->levels;
		}
	}
	return 1;
}

// Detecção de contornos pelos operadores Prewitt
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th) // th = [0.001, 1.000]
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH;
	int i, size;
	int histmax, histthreshold;
	int sumx, sumy;
	int hist[256] = { 0 };

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	size = width * height;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			sumx = datasrc[posA] * -1;
			sumx += datasrc[posD] * -1;
			sumx += datasrc[posF] * -1;

			sumx += datasrc[posC] * +1;
			sumx += datasrc[posE] * +1;
			sumx += datasrc[posH] * +1;
			sumx = sumx / 3; // 3 = 1 + 1 + 1

			sumy = datasrc[posA] * -1;
			sumy += datasrc[posB] * -1;
			sumy += datasrc[posC] * -1;

			sumy += datasrc[posF] * +1;
			sumy += datasrc[posG] * +1;
			sumy += datasrc[posH] * +1;
			sumy = sumy / 3; // 3 = 1 + 1 + 1

			datadst[posX] = (unsigned char)sqrt((double)(sumx * sumx + sumy * sumy));
		}
	}

	// Compute a grey level histogram
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			hist[datadst[y * bytesperline + x * channels]]++;
		}
	}

	// Threshold at the middle of the occupied levels
	histmax = 0;
	for (i = 0; i <= 255; i++)
	{
		histmax += hist[i];

		// th = Prewitt Threshold
		if (histmax >= (((float)size) * th))
			break;
	}
	histthreshold = i;

	// Apply the threshold
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			posX = y * bytesperline + x * channels;

			if (datadst[posX] >= histthreshold)
				datadst[posX] = 255;
			else
				datadst[posX] = 0;
		}
	}

	return 1;
}

//Desenha bounding-box
int vc_desenha_bounding_box_rgb(IVC* src, OVC* blobs, int numeroBlobs)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	int width = src->width;
	int height = src->height;
	int i, yy, xx;
	long int posk;

	// Verificaçao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->channels != 3))
		return 0;

	//percorre os blobs da imagem e para cada blob vai percorrer a altura e o comprimento da sua bounding box
	for (i = 0; i < numeroBlobs; i++)
	{
		//percorre a altura da box
		for (yy = blobs[i].y; yy <= blobs[i].y + blobs[i].height; yy++)
		{
			//percorre a largura da box
			for (xx = blobs[i].x; xx <= blobs[i].x + blobs[i].width; xx++)
			{

				posk = yy * bytesperline_src + xx * channels_src;
				//condiçao para colocar a 255 apenas os pixeis do limite da caixa
				if (yy == blobs[i].y || yy == blobs[i].y + blobs[i].height || xx == blobs[i].x || xx == blobs[i].x + blobs[i].width)
				{
					datasrc[posk] = 255;
					datasrc[posk + 1] = 255;
					datasrc[posk + 2] = 255;
				}
			}
		}
	}

	return 1;
}

//Desenha o centro de massa
int vc_desenha_centro_massa_rgb(IVC* src, OVC* blobs, int numeroBlobs)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	int width = src->width;
	int height = src->height;
	int i, yy, xx;
	long int posk;

	// Verificaçao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->channels != 3))
		return 0;

	// percorre os blobs da imagem e para cada blob vai percorrer a altura
	//e o comprimento de uma area a volta das coordenadas do centro de massa
	for (i = 0; i < numeroBlobs; i++)
	{

		for (yy = blobs[i].yc - 3; yy <= blobs[i].yc + 3; yy++)
		{

			for (xx = blobs[i].xc - 3; xx <= blobs[i].xc + 3; xx++)
			{

				posk = yy * bytesperline_src + xx * channels_src;

				datasrc[posk] = 255;
				datasrc[posk + 1] = 255;
				datasrc[posk + 2] = 255;
			}
		}
	}

	return 1;
}

int vc_draw_bounding_box(int x, int y, int largura, int altura, IVC *isaida)
{
	unsigned char *datadst = (unsigned char *)isaida->data;
	int bytesperline_dst = isaida->width * isaida->channels;
	int channels_dst = isaida->channels;
	int width_dst = isaida->width;
	int height_dst = isaida->height;
	long int pos;

	for (int i = x; i < x + largura; i++)
	{
		pos = y * bytesperline_dst + i * channels_dst;
		datadst[pos] = 0;
		datadst[pos + 1] = 0;
		datadst[pos + 2] = 0;
		pos = (y + altura) * bytesperline_dst + i * channels_dst;
		datadst[pos] = 0;
		datadst[pos + 1] = 0;
		datadst[pos + 2] = 0;
	}
	for (int i = y; i < y + altura; i++)
	{
		pos = i * bytesperline_dst + x * channels_dst;
		datadst[pos] = 0;
		datadst[pos + 1] = 0;
		datadst[pos + 2] = 0;
		pos = i * bytesperline_dst + (x + largura)*channels_dst;
		datadst[pos] = 0;
		datadst[pos + 1] = 0;
		datadst[pos + 2] = 0;
	}
	return 1;
}

int vc_center_of_mass(int x, int y, int xc, int yc, int largura, int altura, IVC *isaida)
{	
	unsigned char *datadst = (unsigned char *)isaida->data;
	int bytesperline_dst = isaida->width * isaida->channels;
	int channels_dst = isaida->channels;
	int width_dst = isaida->width;
	int height_dst = isaida->height;
	long int pos;

	for (int i = x; i < x + largura; i++)
	{
		for (int j = y; j < y + altura; j++)
		{
			if (xc == i && yc == j)
			{
				pos = j * bytesperline_dst + i * channels_dst;
				datadst[pos] = 0;
				datadst[pos + 1] = 0;
				datadst[pos + 2] = 0;
				for (int k = 1; k < 6; k++)
				{
					pos = (j - k)*bytesperline_dst + i * channels_dst;
					datadst[pos] = 0;
					datadst[pos + 1] = 0;
					datadst[pos + 2] = 0;
					pos = (j + k) * bytesperline_dst + i * channels_dst;
					datadst[pos] = 0;
					datadst[pos + 1] = 0;
					datadst[pos + 2] = 0;
					pos = j * bytesperline_dst + (i + k)*channels_dst;
					datadst[pos] = 0;
					datadst[pos + 1] = 0;
					datadst[pos + 2] = 0;
					pos = j * bytesperline_dst + (i - k)*channels_dst;
					datadst[pos] = 0;
					datadst[pos + 1] = 0;
					datadst[pos + 2] = 0;
				}
			}
		}
	}

	return 1;
}

//não está a dar
int vc_gray_histogram_equalization(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	long int pos;
	int x, y, i, k, j;
	//int n = srcdst->width*srcdst->height;
	int ni[256] = { 0 };
	float pdf[256], cdf[256], cdfmin;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL)) return 0;
	if (channels != 3) return 0;

	for (i = 0; i < width*height; i++)
	{
		ni[data[i]]++;
	}

	for (k = 0; k < 256; k++)
	{
		pdf[k] = ni[k] / (width*height);

		if (k == 0)
		{
			cdf[k] = pdf[k];
		}
		else
		{
			cdf[k] = pdf[k] + cdf[k - 1];
		}
	}

	cdfmin = 1;
	for (j = 0; j < 256; j++)
	{
		if (cdf[j] != 0 && cdf[j] < cdfmin)
		{
			cdfmin = cdf[j];
		}
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = (cdf[data[pos]] - cdfmin) / (1 - cdfmin) * 255;
		}
	}

	return 1;
}


int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH;
	int sum;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			sum = datasrc[posA] * +1;
			sum += datasrc[posB] * +1;
			sum += datasrc[posC] * +1;
			sum += datasrc[posD] * +1;
			sum += datasrc[posX] * +1;
			sum += datasrc[posE] * +1;
			sum += datasrc[posF] * +1;
			sum += datasrc[posG] * +1;
			sum += datasrc[posH] * +1;

			datadst[posX] = (unsigned char)(sum / 9);
		}
	}
	return 1;
}

int vc_gray_lowpass_median_filter(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i, j;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH;
	int mediana[9];
	int aux;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			mediana[0] = datasrc[posA];
			mediana[1] = datasrc[posB];
			mediana[2] = datasrc[posC];
			mediana[3] = datasrc[posD];
			mediana[4] = datasrc[posX];
			mediana[5] = datasrc[posE];
			mediana[6] = datasrc[posF];
			mediana[7] = datasrc[posG];
			mediana[8] = datasrc[posH];

			for (i = 0; i < 9 - 1; i++)
			{
				for (j = 0; j < i + 1; j++)
				{
					if (mediana[i] > mediana[j])
					{
						aux = mediana[i];
						mediana[i] = mediana[j];
						mediana[j] = aux;
					}
				}
			}
			datadst[posX] = (unsigned char)mediana[4];
		}
	}
	return 1;
}

int vc_gray_highpass_filter(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posB, posD, posE, posG;
	int sum;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posB = (y - 1) * bytesperline + x * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;

			sum = datasrc[posB] * -1;
			sum += datasrc[posD] * -1;
			sum += datasrc[posX] * 4;
			sum += datasrc[posE] * -1;
			sum += datasrc[posG] * -1;

			datadst[posX] = (unsigned char)(sum / 6);
		}
	}
	return 1;
}


int dominant_color(IVC *image, int largura,int altura)
{
	unsigned char *datadst = (unsigned char *)image->data;
	int bytesperline_dst = image->width * image->channels;
	int channels_dst = image->channels;
	int width_dst = image->width;
	int height_dst = image->height;
	int aux;
	long int pos = altura * bytesperline_dst + largura * channels_dst;


	//caso se verifique que é maior que 100 na posição defenida
	if (datadst[pos] > 100) //assume azul
	{
		aux = 1;
	}
	else //assume que nao é azul
	{
		aux = 0;
	}
	return aux;
}

int vc_stop_forbiden_distinction(IVC *image, int xc, int yc)
{
	unsigned char *datadst = (unsigned char *)image->data;
	int bytesperline_dst = image->width * image->channels;
	int channels_dst = image->channels;
	int width_dst = image->width;
	int height_dst = image->height;
	int aux;
	long int pos = yc * bytesperline_dst + xc * channels_dst;
	
	
	if (datadst[pos + 2 - 100] > 100) //caso se verifique maior que 100 é SINAL PROIBIDO
	{
		aux = 1;
	}
	else //assume que é stop
	{
		aux = 0;
	}
	return aux;
}

int vc_arrows_distinction(IVC *image, int xc, int yc)
{
	unsigned char *datadst = (unsigned char *)image->data;
	int bytesperline_dst = image->width * image->channels;
	int channels_dst = image->channels;
	int width_dst = image->width;
	int height_dst = image->height;
	int aux;
	long int pos = (yc/2-yc/4) * bytesperline_dst + (xc/2-xc/4) * channels_dst;
	

	//caso se verifique que é maior que 100 na posição defenida
	//é virar esquerda
	if (datadst[pos] > 100)
	{
		aux = 1;
	}
	else //caso nao,é virar á direita
	{
		aux = 0;
	}
	return aux;
}