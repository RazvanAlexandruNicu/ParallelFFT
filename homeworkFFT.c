#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>
typedef double complex cplx;

char *inputFile;
char *outputFile;
int numThreads;
FILE *input, *output;
int N;
double PI;

double* inputValuesDouble;
cplx* inputValues;
cplx* outputValues;
cplx* out;
pthread_barrier_t barrier;
pthread_barrier_t barrier2;
cplx *tempout;
cplx *tempinp;

/*
	Open the input and output files
*/

void openFiles()
{
	input = fopen(inputFile, "rt");
	if(!input)
	{
		return;
	}
	output = fopen(outputFile, "wt");
	if(!output)
	{
		return;
	}
}

/*
	Parse the arguments given
*/

void parseArguments(char* argv[])
{
	inputFile = argv[1];
	outputFile =argv[2];
	numThreads = atoi(argv[3]);
	openFiles();
}

/*
	Read the values from the input file and store
	them into a proper data structure
	(an array with complex type elements)
*/

void readValues()
{
	int ret, i;
	ret = fscanf(input, "%d", &N);
	if(!ret)
	{
		return;
	}
	inputValues = (cplx*)calloc(N, sizeof(cplx));
	if(!inputValues)
	{
		return;
	}
	inputValuesDouble = (double*)calloc(N, sizeof(double));
	for(i = 0; i < N; i++)
	{
		ret = fscanf(input, "%lf", &inputValuesDouble[i]);
		if(!ret)
		{
			return;
		}
		inputValues[i] = inputValuesDouble[i];
	}
}

/*
	FFT function - Rosetta version
*/

void _fft(cplx buf[], cplx out[], int n, int step)
{
	if (step < n) {
		_fft(out, buf, n, step * 2);
		_fft(out + step, buf + step, n, step * 2);
 
		for (int i = 0; i < n; i += 2 * step) {
			cplx t = cexp(-I * PI * i / n) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + n)/2] = out[i] - t;
		}
	}
}


/*
	Thread function multiplexing for 1, 2 and 4 threads.
*/
void* threadFunction(void *var)
{
	int thread_id = *(int*)var;
	int start, end;
	start = thread_id * N / numThreads;
	end = (thread_id + 1) * N / numThreads;
	switch(numThreads)
	{
		/*
			1 thread
		*/
		case(1):
			_fft(inputValues, out, N, 1);
			break;
		/*
			2 threads
		*/
		case(2):
			switch(thread_id)
			{	
				case(0):
					_fft(out, inputValues, N, 2);
					break;
				case(1):
					_fft(out + 1, inputValues + 1, N, 2);
					break;
			}
			/*
				Use the barrier in order to be sure that the previous
				recursive calls are finished.
			*/

			pthread_barrier_wait(&barrier);
			
			/*
				After the recursive sub-problems are solved, compute
				the final values.
			*/

			for (int i = start; i < end; i += 2)
			{
				cplx t = cexp(-I * PI * i / N) * out[i + 1];
				inputValues[i / 2]     = out[i] + t;
				inputValues[(i + N)/2] = out[i] - t;
			}

			break;

		/*
			4 threads
		*/
		case(4):
			switch(thread_id)
			{	
				case(0):
					_fft(inputValues, out, N, 4);
					break;
				case(1):
					_fft(inputValues + 1, out + 1, N, 4);
					break;
				case(2):
					_fft(inputValues + 2, out + 2, N, 4);
					break;
				case(3):
					_fft(inputValues + 3, out + 3, N, 4);
					break;
				default:
					break;
			}

			/*
				Use the barrier in order to be sure that the previous
				recursive calls are finished.
			*/

			pthread_barrier_wait(&barrier);

			/*
				After the recursive sub-problems are solved, compute
				the intermediar values.
			*/
			switch(thread_id)
			{	
				case(0):
					for (int i = 0; i < N; i += 4)
					{
						cplx t = cexp(-I * PI * i / N) * inputValues[i + 2];
						out[i / 2]     = inputValues[i] + t;
						out[(i + N)/2] = inputValues[i] - t;
					}
					break;
				case(1):
					tempout = out + 1;
					tempinp = inputValues + 1;
					for (int i = 0; i < N; i += 4)
					{
						cplx t = cexp(-I * PI * i / N) * tempinp[i + 2];
						tempout[i / 2]     = tempinp[i] + t;
						tempout[(i + N)/2] = tempinp[i] - t;
					}
					break;
				default:
					break;
			}

			/*
				Use the barrier in order to be sure that the previous sub-problems
				are solved
			*/

			pthread_barrier_wait(&barrier2);

			/*
				After the recursive sub-problems are solved, compute
				the final values.
			*/
			if(thread_id == 0)
			{
				for (int i = 0; i < N; i += 2)
				{
					cplx t = cexp(-I * PI * i / N) * out[i + 1];
					inputValues[i / 2]     = out[i] + t;
					inputValues[(i + N)/2] = out[i] - t;
				}
			}
			
			break;
	}

	return NULL;
}

/*
	Function that creates the threads and runs them
*/

void createThreads(pthread_t* tid, int* thread_id)
{
	int i;
	for(i = 0;i < numThreads; i++)
		thread_id[i] = i;

	for(i = 0; i < numThreads; i++) {
		pthread_create(&(tid[i]), NULL, threadFunction, &(thread_id[i]));
	}

	for(i = 0; i < numThreads; i++) {
		pthread_join(tid[i], NULL);
	}
}

/*
	Prints the output values in the proper format
*/

void printInput()
{
	int k;
	fprintf(output, "%d\n", N);
	for(k = 0; k < N; k++)
	{
		fprintf(output, "%lf %lf\n", creal(inputValues[k]), cimag(inputValues[k]));
	}
	fclose(input);
	fclose(output);
}

/*
	Frees the allocated memory on the heap
*/

void freeMemory()
{
	free(outputValues);
	free(inputValues);
	fclose(input);
	fclose(output);
}

int main(int argc, char * argv[]) {
	parseArguments(argv);
	readValues();
	
	PI = atan2(1, 1) * 4;
	pthread_t tid[numThreads];
	int thread_id[numThreads];
	pthread_barrier_init(&barrier, NULL, numThreads);
	pthread_barrier_init(&barrier2, NULL, numThreads);

	out = (cplx*)calloc(N, sizeof(cplx));
	if(!out)
	{
		return -1;
	}
	createThreads(tid, thread_id);
	printInput();
	freeMemory();
	pthread_barrier_destroy(&barrier);
	pthread_barrier_destroy(&barrier2);
	return 0;
}
