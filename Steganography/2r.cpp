#include"mpi.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<conio.h>

#define MAX_IMAGE_HEIGHT 512
#define MAX_IMAGE_WIDTH 512
#define text_size 30

int main(int argc, char* argv[])
{
	int id, size, i, j, height, width, padding, pixels_per_process, text_sz;
	static unsigned char header[54], r[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], g[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], b[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], final_r[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], final_g[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], final_b[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], intermediate_r[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], intermediate_g[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], intermediate_b[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], text[(MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH) / 8], text1[(MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH) / 8];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	pixels_per_process = MAX_IMAGE_HEIGHT / size;

	text_sz = text_size / size;

	char *image_input = "a1.bmp";
	char *text_output = "abc_out.txt";

	if (id == 0)
	{
		printf("Input image: %s\n", image_input);

		FILE *fd, *store_pixels;

		fd = fopen(image_input, "rb");

		if (fd == NULL)
		{
			printf("Error: fopen failed for %s\n", image_input);
			return 0;
		}

		store_pixels = fopen("store_output_pixels.txt", "w+");

		if (store_pixels == NULL)
		{
			printf("Error: fopen failed for store_output_pixels\n");
			return 0;
		}

		/* Read header for height, width information */

		fread(header, sizeof(unsigned char), 54, fd);

		width = *(int*)&header[18];
		height = *(int*)&header[22];
		padding = 0;

		while ((width * 3 + padding) % 4 != 0)
		{
			padding++;
		}

		printf("Dimensions of %s: %d x %d pixels\n", image_input, height, width);
		printf("Image padding: %d\n", padding);

		static unsigned char image[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH][3];

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

	}


	MPI_Bcast(r, MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(g, MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(b, MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);

	for (i = id*pixels_per_process; i < (id + 1)*pixels_per_process; i++)
	{
		for (j = 0; j < MAX_IMAGE_WIDTH; j++)
		{
			intermediate_r[i - (id*pixels_per_process)][j] = r[i][j];
			intermediate_g[i - (id*pixels_per_process)][j] = g[i][j];
			intermediate_b[i - (id*pixels_per_process)][j] = b[i][j];
		}
	}

	char bits[48];
	int index = 0;



	for (j = 0; j < 48; j++)
	{
		bits[j] = intermediate_b[0][j] & 0x01;
	}




	int c = 0;

	for (j = 0; j < 6; j++)
	{
		c = 0;
		for (i = 0; i < 8; i++)
		{
			c = c << 1;
			c = c | bits[j * 8 + 7 - i];
		}
		printf("\nID:%d  c: %c", id, c);

		text[index++] = c;
	}

	MPI_Gather(text, text_sz, MPI_CHAR, text1, text_sz, MPI_CHAR, 0, MPI_COMM_WORLD);

	if (id == 0)
	{


		FILE *f1 = fopen(text_output, "w+");

		if (f1 == NULL)
		{
			printf("Error: fopen failed for %s\n", text_output);
			return 0;
		}

		fwrite(text1, sizeof(char), text_size, f1);
		fclose(f1);

	}

	MPI_Finalize();
	return 0;
}
