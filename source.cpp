/*SeyyedMohammad Hosseini
Seyyedmohammad.ir
Iasbs.ac.ir */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
using namespace std;

#include "mpi.h"
#pragma comment(lib, "msmpi.lib")

#define ROUND(a) ((int) (a + 0.5)) 

FILE * fp;
char *filename = "new1.bmp";    //bmp version
unsigned char *img = NULL;


typedef struct {
	float x, y;
} Complex;

Complex complexSquare(Complex c){
	Complex cSq;
	cSq.x = c.x * c.x - c.y * c.y;
	cSq.y = 2 * c.x * c.y;
	return cSq;
}

int iterate(Complex zInit, int maxIter){
	Complex z = zInit;
	int cnt = 0;
	while ((z.x * z.x + z.y * z.y <= 4) && (cnt < maxIter)){
		z = complexSquare(z);
		z.x += zInit.x;
		z.y += zInit.y;
		cnt++;
	}
	return cnt;
}

void madelbrot(int nx, int min_ny, int ny, int maxIter, float realMin, float realMax, float imagMin, float imagMax, int myCounter){
	static unsigned char color[3];
	float realInc = (realMax - realMin) / nx;
	float imagInc = (imagMax - imagMin) / ny;
	int max;
	Complex z;
	int x, y, w, h;
	int cnt;
	for (x = 0, z.x = realMin; x < nx; x++, z.x += realInc)
	for (y = min_ny, z.y = imagMin; y < ny; y++, z.y += imagInc){
		cnt = iterate(z, maxIter);
		//using cnt to calculate pixel's colour
		w = x;
		h = (ny - 1) - y;
		if (cnt == maxIter)
		{
			color[0] = 0;
			color[1] = 0;
			color[2] = 0;
		}
		else{
			double c = 3 * log((double)cnt) / log(maxIter - 1.0);
			if (c < 1)
			{
				color[0] = 255 * c;
				color[1] = 0;
				color[2] = 0;
			}
			else if (c < 2)
			{
				color[0] = 255;
				color[1] = 255 * (c - 1);
				color[2] = 0;
			}
			else
			{
				color[0] = 255;
				color[1] = 255;
				color[2] = 255 * (c - 2);
			}
		}
		//putting colours in corresponding pixel (w,h)
		img[myCounter + (w + h*nx) * 3 + 2] = color[0];
		img[myCounter + (w + h*nx) * 3 + 1] = color[1];
		img[myCounter + (w + h*nx) * 3 + 0] = color[2];
	}
}

void main(int argc, char *argv[]){
	int myid, numprocs;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	//initial setting
	const int MaxColorComponentValue = 255;
	const int iXmax = 900;
	const int iYmax = 600;
	const int iterations = 1000;
	const int realMin = -2.0;
	const int realMax = 1.0;
	const int imagMin = -1.0;
	const int imagMax = 1.0;
	int _every_proccess;
	float _every_d;
	int filesize = 54 + 3 * iXmax * iYmax;
	if (img) delete img;


	img = new unsigned char[3 * iXmax * iYmax];
	memset(img, 0, sizeof(img));

	//--- Parallel
	_every_proccess = 600 / numprocs;
	_every_d = (float)2 / numprocs;
	if (myid == 0){
		madelbrot(900, 0, _every_proccess, iterations, realMin, 1, 1 - _every_d, 1, 0);
	}
	else{
		madelbrot(900, myid*_every_proccess, (myid + 1)*_every_proccess, iterations, realMin, realMax, (float)1 - (myid + 1)*_every_d, 1, 900 * myid*_every_proccess * 3);
		for (int j = myid*_every_proccess; j < (myid + 1)*_every_proccess; j++)
			MPI_Send(&img[j * 900 * 3], 900 * 3, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD);
	}
	if (myid == 0){

		unsigned char bmpfileheader[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
		unsigned char bmpinfoheader[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0 };
		unsigned char bmppad[3] = { 0, 0, 0 };

		bmpfileheader[2] = (unsigned char)(filesize);
		bmpfileheader[3] = (unsigned char)(filesize >> 8);
		bmpfileheader[4] = (unsigned char)(filesize >> 16);
		bmpfileheader[5] = (unsigned char)(filesize >> 24);

		bmpinfoheader[4] = (unsigned char)(iXmax);
		bmpinfoheader[5] = (unsigned char)(iXmax >> 8);
		bmpinfoheader[6] = (unsigned char)(iXmax >> 16);
		bmpinfoheader[7] = (unsigned char)(iXmax >> 24);
		bmpinfoheader[8] = (unsigned char)(iYmax);
		bmpinfoheader[9] = (unsigned char)(iYmax >> 8);
		bmpinfoheader[10] = (unsigned char)(iYmax >> 16);
		bmpinfoheader[11] = (unsigned char)(iYmax >> 24);
		fp = fopen(filename, "wb");
		fwrite(bmpfileheader, 1, 14, fp);
		fwrite(bmpinfoheader, 1, 40, fp);


		MPI_Status status;

		for (int i = 1; i < numprocs; i++){
			for (int j = i*_every_proccess; j < (i + 1)*_every_proccess;j++)
				MPI_Recv(&img[j * 900 * 3], 900 * 3, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD, &status);
		}
			
		for (int i = 0; i < iYmax; i++){
			fwrite(img + (iXmax *(iYmax - i - 1) * 3), 3, iXmax, fp);
			fwrite(bmppad, 1, (4 - (iXmax * 3) % 4) % 4, fp);
		}
		fclose(fp);
	}
	MPI_Finalize();
}
