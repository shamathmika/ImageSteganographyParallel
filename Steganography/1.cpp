#include"mpi.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<conio.h>

#define MAX_IMG_HEIGHT 512
#define MAX_IMAGE_WIDTH 512

int main(int argc, char* argv[])
{
	int id, size, text_size, i, j, height, width, padding, pixels_per_process;
	static unsigned char header[54], r[512][512], g[512][512], b[512][512], final_r[512][512], final_g[512][512], final_b[512][512], intermediate_r[512][512], intermediate_g[512][512], intermediate_b[512][512], text[512 * 512];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	pixels_per_process = 512 / size;

	if (id == 0) {

		char *image_input = "b.bmp";
		char *text_input = "abc.txt";
		char *image_output = "a1.bmp";

		FILE *fd, *store_pixels;
		printf("Input image: %s\n", image_input);

		fd = fopen(image_input, "rb");
		if (fd == NULL) {
			printf("Error: fopen failed for %s\n", image_input);
			return 0;
		}

		store_pixels = fopen("store_pixels.txt", "w+");
		if (store_pixels == NULL) {
			printf("Error: fopen failed for store_pixels\n");
			return 0;
		}

		/* Read header for height, width information */

		fread(header, sizeof(unsigned char), 54, fd);

		width = *(int*)&header[18];
		height = *(int*)&header[22];
		padding = 0;

		while ((width * 3 + padding) % 4 != 0) {
			padding++;
		}

		printf("Image width: %d\n", width);
		printf("Image height: %d\n", height);
		printf("Image padding: %d\n", padding);

		static unsigned char image[512][512][3];

		for (i = 0, j = 0; i<height; j++)
		{
			fread(image[i][j], sizeof(unsigned char), 3, fd);
			fprintf(store_pixels, "image[%d][%d] = %d %d %d\n", i, j, image[i][j][0], image[i][j][1], image[i][j][2]);
			r[i][j] = image[i][j][0];
			g[i][j] = image[i][j][1];
			b[i][j] = image[i][j][2];
			if (j == (width - 1))
			{
				j = -1;
				i++;
			}
		}
		fclose(fd);


		/* Reading the text file */

		FILE *f = fopen(text_input, "r");

		if (fd == NULL) {
			printf("Error: fopen failed for %s\n", text_input);
			return 0;
		}

		fseek(f, 0, SEEK_END);
		text_size = ftell(f);
		fprintf(stdout, "Size of %s: %d\n", text_input, text_size);
		fseek(f, 0, SEEK_SET);
		fread(text, sizeof(unsigned char), text_size, fd);
		text[text_size] = '\0';
		fprintf(stdout, "Text in %s: %s\n\n", text_input, text);
	}


	MPI_Bcast(r, 512 * 512, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(g, 512 * 512, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(b, 512 * 512, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(&text_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(text, text_size, MPI_CHAR, 0, MPI_COMM_WORLD);

	for (i = id*pixels_per_process; i < (id + 1)*pixels_per_process; i++) {
		for (j = 0; j < 512; j++) {

			intermediate_r[i - (id*pixels_per_process)][j] = r[i][j];
			intermediate_g[i - (id*pixels_per_process)][j] = g[i][j];
			intermediate_b[i - (id*pixels_per_process)][j] = b[i][j];

		}
	}

	text_size /= size;
	int count = 0;
	int x = id * text_size;
	int x1 = x;
	char c = text[x];
	int mask = 1;
	int flag = 0;

	for (i = id*pixels_per_process; i < (id + 1)*pixels_per_process; i++) {
		for (j = 0; j < 512; j++) {


			if (count == 8)
			{
				if (x < x1*text_size + text_size)
				{
					c = text[++x];
					count = 0;
					mask = 1;
				}
				else
				{
					flag = 1;
					break;
				}
			}
			char ch = c & mask;
			intermediate_b[i - (id*pixels_per_process)][j] = intermediate_b[i - (id*pixels_per_process)][j] & 0xFE;
			intermediate_b[i - (id*pixels_per_process)][j] = intermediate_b[i - (id*pixels_per_process)][j] | ch;

			mask = mask << 1;
			count++;

		}
		if (flag == 1)
		{
			break;
		}
	}
	MPI_Gather(intermediate_r, pixels_per_process * 512, MPI_CHAR, final_r, pixels_per_process * 512, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Gather(intermediate_g, pixels_per_process * 512, MPI_CHAR, final_g, pixels_per_process * 512, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Gather(intermediate_b, pixels_per_process * 512, MPI_CHAR, final_b, pixels_per_process * 512, MPI_CHAR, 0, MPI_COMM_WORLD);


	if (id == 0) {

		unsigned char black1[512][512][3];

		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				black1[i][j][0] = final_r[i][j];
				black1[i][j][1] = final_g[i][j];
				black1[i][j][2] = final_b[i][j];
			}
		}

		printf("\nHeight after priniting : %d--Width after printing %d\n", height, width);

		/*	 //to write array into an image "img_out.bmp"*/

		FILE *fd1;


		fd1 = fopen("a1.bmp", "wb");
		if (fd1 == NULL)
		{
			printf("Error: fopen failed\n");
			return 0;
		}

		fwrite(header, sizeof(unsigned char), 54, fd1);
		for (i = 0; i<height; i++)
		{
			for (j = 0; j<width; j++)
			{
				fwrite(black1[i][j], sizeof(unsigned char), 3, fd1);
			}
		}

		fclose(fd1);

	}

	MPI_Finalize();
	return 0;
}
