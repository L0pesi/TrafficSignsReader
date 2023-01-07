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
	// V�deo
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
	
	/* Leitura de v�deo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video-tp2.avi dever� estar localizado no mesmo direct�rio que o ficheiro de c�digo fonte.
	*/
	//capture.open(videofile);

	/* Em alternativa, abrir captura de v�deo pela Webcam #0 */
	capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi poss�vel abrir o ficheiro de v�deo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de v�deo!\n";
		return 1;
	}

	/* N�mero total de frames no v�deo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do v�deo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolu��o do v�deo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o v�deo */
	cv::namedWindow("VC - TP2", cv::WINDOW_AUTOSIZE);	

	cv::Mat frame;

	while (key != 'q') 
	{
		/* Leitura de uma frame do v�deo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* N�mero da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		/* Exemplo de inser��o texto na frame */
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
		// Executa uma fun��o da nossa biblioteca vc	
		memcpy(image4->data, image->data, image4->width*image4->height * 3); //c�pia imagem original
		
		//Resolu��o do espa�o de cor parte 1
		vc_rgb_to_hsv(image); //tranforma�ao de rgb para hsv

		//Segmenta��o das imagens parte 1
		vc_hsv_segmentation(image, 0, 360, 30, 100, 30, 100); 
		//segmenta��o hsv de todos os sinais

		//Resolu��o do espa�o de cor parte 2
		vc_rgb_to_gray(image, image2);//Transforma��o de rgb para escala de cinzentos

		//Segmenta��o das imagens parte 2
		vc_binary_erode(image2, image3, 9);//Eros�o

		//Etiquetagem - �rea, per�metro
		OVC* nobjetos = vc_binary_blob_labelling(image3, image2, &numero);//

		if (numero != 0) {
			vc_binary_blob_info(image2, nobjetos, numero);
			for (int i = 0; i < numero; i++)
			{
				//Caso verifique-se que a �reas das imagens � maior que 15000,
				//� implementado a bounding box e o centro de massa nas imagens e,
				//� calculado cada um dos termos result.

				if (nobjetos[i].area > 15000) {  
					vc_draw_bounding_box(nobjetos[i].x, nobjetos[i].y, nobjetos[i].width, nobjetos[i].height, image4);
					vc_center_of_mass(nobjetos[i].x, nobjetos[i].y, nobjetos[i].xc, nobjetos[i].yc, nobjetos[i].width, nobjetos[i].height, image4);
					
					result1 = dominant_color(image4, nobjetos[i].width, nobjetos[i].height);//Na posi��o selecionada verifica a cor dominante
					result2 = vc_stop_forbiden_distinction(image4, nobjetos[i].xc, nobjetos[i].yc);//verifica na posi��o definida a existencia dos elementos identificativos
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
		// Liberta a mem�ria da imagem IVC que havia sido criada
		vc_image_free(image);
		vc_image_free(image2);
		vc_image_free(image3);
		vc_image_free(image4);
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//Caso o resultado de cada result se verifique em cada condi��o,
		//� apresentado na imagem a identifica��o do sinal.
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//result1 calcula a cor dominante dos sinais,caso a condi��o seja azul passa para a condi��o result4,
		//caso seja vermelho passa para a condi��o result2
		if (result1 == 1) 
		{		

			//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			//result4 faz o seguinte calculo: �rea/perimetro do circulo,
			//caso o resultado esteja entre 20 e 100 � circulo. 
			if ((result4 > 20) && (result4 < 100))
			{
				//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				//result 5 faz a distin��o das setas, atrav�s da sua posi��o,
				//se data[pos]>100 � virar esquerda, se n�o virar direita
				if (result5 == 1)
				{
					cv::putText(frame, "Virar esquerda", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				}
				else
				{
					cv::putText(frame, "Virar direita", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
				}
			}
			else //caso n�o seja quadrado
			{
				//result3 faz o seguinte calculo:altura/largura
				//caso result 3 seja >0.9 � auto-estrada,
				//caso <0.8 � carro
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
			//result2 faz a distin��o dos sinais stop e probido, atrav�s da posi��o,
			//do centro de massa,se verificar datadst[pos]>100, � proibido
			if (result2 == 1)
			{
				cv::putText(frame, "Sentido Proibido", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
			}
			else //caso nao, � stop
			{
				cv::putText(frame, "STOP", cv::Point(20, 125), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
			}
		}		
		/* Exibe a frame */
		cv::imshow("VC - TP2", frame);

		/* Sai da aplica��o, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}
	


	/* Fecha a janela */
	cv::destroyWindow("VC - TP2");

	/* Fecha o ficheiro de v�deo */
	capture.release();

	return 0;
}