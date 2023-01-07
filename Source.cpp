#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}


int main(void) {
	// Vídeo
	//char videofile[20] = "video-tp2.avi";
	cv::VideoCapture capture; //abre video camara ou video
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	// Outros
	std::string str;
	int key = 0;	
	
	/* Leitura de vídeo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video-tp2.avi deverá estar localizado no mesmo directório que o ficheiro de código fonte.
	*/
	//capture.open(videofile);

	/* Em alternativa, abrir captura de vídeo pela Webcam #0 */
	capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi possível abrir o ficheiro de vídeo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	/* Número total de frames no vídeo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do vídeo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolução do vídeo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o vídeo */
	cv::namedWindow("VC - TP2", cv::WINDOW_AUTOSIZE);	

	cv::Mat frame;

	while (key != 'q') 
	{
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inserção texto na frame */
		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
		
		// Cria uma novas imagens IVC
		IVC* image = vc_image_new(video.width, video.height, 3, 255);
		IVC* image2 = vc_image_new(image->width, image->height, 1, image->levels);
		IVC* image3 = vc_image_new(image->width, image->height, 1, image->levels);
		IVC* image4 = vc_image_new(image->width, image->height, 3, image->levels);

		int numero = 0;
		float result1, result2, result3, result4, result5;


		//result.create(frame.rows, frame.cols, CV_32FC1);
		// Cria uma nova imagem IVC
		//IVC *image = vc_image_new(video.width, video.height, 3, 255);
		// Copia dados de imagem da estrutura cv::Mat para uma estrutura IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);
		// Executa uma função da nossa biblioteca vc	
		memcpy(image4->data, image->data, image4->width*image4->height * 3); //cópia imagem original
		
		//Resolução do espaço de cor parte 1
		vc_rgb_to_hsv(image); //tranformaçao de rgb para hsv

		//Segmentação das imagens parte 1
		vc_hsv_segmentation(image, 0, 360, 30, 100, 30, 100); 
		//segmentação hsv de todos os sinais

		//Resolução do espaço de cor parte 2
		vc_rgb_to_gray(image, image2);//Transformação de rgb para escala de cinzentos

		//Segmentação das imagens parte 2
		vc_binary_erode(image2, image3, 9);//Erosão

		//Etiquetagem - Área, perímetro
		OVC* nobjetos = vc_binary_blob_labelling(image3, image2, &numero);//

		if (numero != 0) {
			vc_binary_blob_info(image2, nobjetos, numero);
			for (int i = 0; i < numero; i++)
			{
				//Caso verifique-se que a áreas das imagens é maior que 15000,
				//é implementado a bounding box e o centro de massa nas imagens e,
				//é calculado cada um dos termos result.

				if (nobjetos[i].area > 15000) {  
					vc_draw_bounding_box(nobjetos[i].x, nobjetos[i].y, nobjetos[i].width, nobjetos[i].height, image4);
					vc_center_of_mass(nobjetos[i].x, nobjetos[i].y, nobjetos[i].xc, nobjetos[i].yc, nobjetos[i].width, nobjetos[i].height, image4);
					
					result1 = dominant_color(image4, nobjetos[i].width, nobjetos[i].height);//Na posição selecionada verifica a cor dominante
					result2 = vc_stop_forbiden_distinction(image4, nobjetos[i].xc, nobjetos[i].yc);//verifica na posição definida a existencia dos elementos identificativos
					result3 = (float)(nobjetos[i].width / nobjetos[i].height); //faz a divisao da largura a dividir pela altura
					result4 = (float)(nobjetos[i].width*nobjetos[i].height) / (float)(2*3.14*(nobjetos[i].yc/2)); //area do circulo / perimetro do circulo
					result5 = vc_arrows_distinction(image4, nobjetos[i].xc, nobjetos[i].yc); //distingue virar direita/esquerda
				}
			}
			free(nobjetos);
		}
		// Copia dados de imagem da estrutura IVC para uma estrutura cv::Mat
		//memcpy(frame.data, image->data, video.width * video.height*3);
		memcpy(frame.data, image4->data, video.width * video.height*3);
		// Liberta a memória da imagem IVC que havia sido criada
		vc_image_free(image);
		vc_image_free(image2);
		vc_image_free(image3);
		vc_image_free(image4);
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//Caso o resultado de cada result se verifique em cada condição,
		//é apresentado na imagem a identificação do sinal.
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//result1 calcula a cor dominante dos sinais,caso a condição seja azul passa para a condição result4,
		//caso seja vermelho passa para a condição result2
		if (result1 == 1) 
		{		

			//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			//result4 faz o seguinte calculo: área/perimetro do circulo,
			//caso o resultado esteja entre 20 e 100 é circulo. 
			if ((result4 > 20) && (result4 < 100))
			{
				//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				//result 5 faz a distinção das setas, através da sua posição,
				//se data[pos]>100 é virar esquerda, se não virar direita
				if (result5 == 1)
				{
					cv::putText(frame, "Virar esquerda", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				}
				else
				{
					cv::putText(frame, "Virar direita", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				}
			}
			else //caso não seja quadrado
			{
				//result3 faz o seguinte calculo:altura/largura
				//caso result 3 seja >0.9 é auto-estrada,
				//caso <0.8 é carro
				if (result3 > 0.9)// && (result3 < 1.1))
				{
					cv::putText(frame, "Auto-estrada", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				}
				else //if(result3 < 0.9)
				{
					cv::putText(frame, "Carro", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				}
			}
		}
		else //caso result 1 nao seja cor dominante azul
		{	
			//+++++++++++++++++++++++++++++++++++++++++++++++++++
			//result2 faz a distinção dos sinais stop e probido, através da posição,
			//do centro de massa,se verificar datadst[pos]>100, é proibido
			if (result2 == 1)
			{
				cv::putText(frame, "Sentido Proibido", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
			}
			else //caso nao, é stop
			{
				cv::putText(frame, "STOP", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
			}
		}		
		/* Exibe a frame */
		cv::imshow("VC - TP2", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}
	


	/* Fecha a janela */
	cv::destroyWindow("VC - TP2");

	/* Fecha o ficheiro de vídeo */
	capture.release();

	return 0;
}