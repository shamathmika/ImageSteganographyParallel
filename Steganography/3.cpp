#include"mpi.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<conio.h>
#include<math.h>

#define MAX_IMAGE_HEIGHT 512
#define MAX_IMAGE_WIDTH 512

int main(int argc, char* argv[])
{
	int id, size, text_size, i, j, height, width, padding, pixels_per_process;
	static unsigned char header[54], r[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], g[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], b[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], final_r[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], final_g[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], final_b[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], intermediate_r[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], intermediate_g[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], intermediate_b[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH], text[(MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH) / 8];

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	pixels_per_process = MAX_IMAGE_HEIGHT / size;

	char *image_input = "b.bmp";
	char *text_input = "abc.txt";
	char *image_output = "a1.bmp";
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

		store_pixels = fopen("store_input_pixels.txt", "w+");

		if (store_pixels == NULL)
		{
			printf("Error: fopen failed for store_input_pixels\n");
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


		/* Reading the text file */
		printf("Text file to be hidden: %s\n", text_input);

		FILE *f = fopen(text_input, "r");

		if (f == NULL)
		{
			printf("Error: fopen failed for %s\n", text_input);
			return 0;
		}

		fseek(f, 0, SEEK_END);
		text_size = ftell(f);
		fprintf(stdout, "Size of %s: %d characters\n", text_input, text_size);
		fseek(f, 0, SEEK_SET);
		fread(text, sizeof(unsigned char), text_size, fd);
		text[text_size] = '\0';
		fprintf(stdout, "Text in %s: %s\n\n", text_input, text);
	}


	MPI_Bcast(r, MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(g, MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(b, MAX_IMAGE_HEIGHT * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(&text_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(text, text_size, MPI_CHAR, 0, MPI_COMM_WORLD);

	for (i = id*pixels_per_process; i < (id + 1)*pixels_per_process; i++)
	{
		for (j = 0; j < MAX_IMAGE_WIDTH; j++)
		{
			intermediate_r[i - (id*pixels_per_process)][j] = r[i][j];
			intermediate_g[i - (id*pixels_per_process)][j] = g[i][j];
			intermediate_b[i - (id*pixels_per_process)][j] = b[i][j];
		}
	}

	text_size /= size;
	int bits_written = 0;
	int index = id * text_size;
	int start_index = index;
	char c = text[start_index];
	int mask = 1;
	int flag = 0;

	printf("\n\nID: %d, text_size: %d, index: %d, c: %c|%d, i: %d\n\n", id, text_size, index, c, c, id*pixels_per_process);


	for (i = id*pixels_per_process; i < (id + 1)*pixels_per_process; i++)
	{
		for (j = 0; j < MAX_IMAGE_WIDTH; j++)
		{
			flag = 0;
			if (bits_written == 7)
			{
				if (index < (start_index + text_size - 1))
				{
					c = text[++index];
					bits_written = 0;
					mask = 1;
				}
				else
				{
					flag = 1;
					break;
				}
			}
			else
			{
				char ch = c & mask;
				int m = log(mask) / log(2);
				ch = ch >> m;
				unsigned char temp = intermediate_b[i - (id*pixels_per_process)][j] & 0xFE;
				intermediate_b[i - (id*pixels_per_process)][j] = intermediate_b[i - (id*pixels_per_process)][j] & 0xFE;
				intermediate_b[i - (id*pixels_per_process)][j] = intermediate_b[i - (id*pixels_per_process)][j] | ch;

				// /*
				if (id == 1)
				{
					FILE *o = fopen("write.txt", "a");
					fprintf(o, "\nID: %d  c: %c  mask: %d  ch: %01d\n", id, c, mask, ch);
					fprintf(o, "ID: %d  temp: %d  intermed[%d][%d]: %d\n\n", id, temp, i - (id*pixels_per_process), j, intermediate_b[i - (id*pixels_per_process)][j]);
					fclose(o);
				}
				//*/



				mask = mask << 1;
				bits_written++;
			}
		}
		if (flag == 1)
		{
			break;
		}
	}


	MPI_Gather(intermediate_r, pixels_per_process * MAX_IMAGE_WIDTH, MPI_CHAR, final_r, pixels_per_process * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Gather(intermediate_g, pixels_per_process * MAX_IMAGE_WIDTH, MPI_CHAR, final_g, pixels_per_process * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Gather(intermediate_b, pixels_per_process * MAX_IMAGE_WIDTH, MPI_CHAR, final_b, pixels_per_process * MAX_IMAGE_WIDTH, MPI_CHAR, 0, MPI_COMM_WORLD);


	if (id == 0)
	{
		unsigned char final_image[MAX_IMAGE_HEIGHT][MAX_IMAGE_WIDTH][3];

		for (i = 0; i < height; i++)
		{
			for (j = 0; j < width; j++)
			{
				final_image[i][j][0] = final_r[i][j];
				final_image[i][j][1] = final_g[i][j];
				final_image[i][j][2] = final_b[i][j];
			}
		}

		printf("\nOutput image %s\n", image_output);
		printf("Dimensions of %s: %d x %d pixels\n", image_output, height, width);

		/* Write final_image into image_output */

		FILE *fd1;


		fd1 = fopen(image_output, "wb");
		if (fd1 == NULL)
		{
			printf("Error: fopen failed for %s\n", image_output);
			return 0;
		}

		fwrite(header, sizeof(unsigned char), 54, fd1);
		for (i = 0; i<height; i++)
		{
			for (j = 0; j<width; j++)
			{
				fwrite(final_image[i][j], sizeof(unsigned char), 3, fd1);
			}
		}

		fclose(fd1);


	}

	MPI_Finalize();
	return 0;
}
