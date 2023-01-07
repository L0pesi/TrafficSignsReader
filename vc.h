#define VC_DEBUG
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char* data;
	int width, height;
	int channels;			// Binário/Cinzentos=1; RGB=3
	int levels;				// Binário=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// Área
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Perímetro
	int label;					// Etiqueta
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);

int vc_gray_negative(IVC *srcdst);
int vc_rgb_negative(IVC *srcdst);

int vc_rgb_get_red_gray(IVC *srcdst);
int vc_rgb_get_green_gray(IVC *srcdst);
int vc_rgb_get_blue_gray(IVC *srcdst);

int vc_rgb_to_gray(IVC *src, IVC *dst);

//int vc_rgb_to_hsv(IVC *src, IVC *dst);
int vc_rgb_to_hsv(IVC *srcdst);

int vc_hsv_segmentation(IVC *srcdst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

int vc_scale_gray_to_rgb(IVC *src, IVC *dst);

int vc_gray_to_binary(IVC *srcdst, int threshold);
int vc_gray_to_binary_global_mean(IVC *srcdst);
int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel);

int vc_binary_dilate(IVC *src, IVC *dst, int kernel);
int vc_binary_erode(IVC *src, IVC *dst, int kernel);

//int vc_binary_open(IVC *src, IVC *dst, int sizeerode, int sizedilate);  //é possível usar kerneis diferentes
int vc_binary_open(IVC *src, IVC *dst, int kernel);
//int vc_binary_close(IVC *src, IVC *dst, int sizeerode, int sizedilate);  //é possível usar kerneis diferentes
int vc_binary_close(IVC *src, IVC *dst, int kernel);


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels);
int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs);

int vc_gray_histogram_show(IVC* src, IVC* dst);
int vc_gray_histogram_equalization(IVC* srcdst);

int vc_desenha_bounding_box_rgb(IVC* src, OVC* blobs, int numeroBlobs);
int vc_desenha_centro_massa_rgb(IVC* src, OVC* blobs, int numeroBlobs);

int vc_draw_bounding_box(int x, int y, int largura, int altura, IVC *isaida);
int vc_center_of_mass(int x, int y, int xc, int yc, int largura, int altura, IVC *isaida);

int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th);

int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst);
int vc_gray_lowpass_median_filter(IVC *src, IVC *dst);
int vc_gray_highpass_filter(IVC *src, IVC *dst);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int vc_draw_bounding_box(int x, int y, int largura, int altura, IVC* isaida);
int vc_center_of_mass(int x, int y, int xc, int yc, int largura, int altura, IVC* isaida);
int dominant_color(IVC *image, int largura, int altura);
int vc_stop_forbiden_distinction(IVC *image, int xc, int yc);
int vc_arrows_distinction(IVC *image, int xc, int yc);