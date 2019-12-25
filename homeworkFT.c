#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <complex.h>

char *inputFile;
char *outputFile;
int numThreads;
FILE *input, *output;
int N;
double PI;
typedef double complex cplx;
double* inputValuesDouble;
cplx* inputValues;
cplx* outputValues;

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
	Thread function that computes the FT, each thread having a
	scope of computing in the array of values.
*/
void* threadFunction(void *var)
{
	int thread_id = *(int*)var;
	int start, end;
	start = thread_id * N / numThreads;
	end = (thread_id + 1) * N / numThreads;
	
	for(int k = start; k < end; k++)
	{
		for(int n = 0; n <= N-1; n++)
		{
			cplx c = inputValues[n] * cexp((- I * 2 * PI / N) * k * n);
			outputValues[k] += c;
		}	
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
void printOutput()
{
	int k;
	fprintf(output, "%d\n", N);
	for(k = 0; k < N; k++)
	{
		fprintf(output, "%lf %lf\n", creal(outputValues[k]), cimag(outputValues[k]));
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
}

int main(int argc, char * argv[]) {
	parseArguments(argv);
	readValues();
	PI = atan2(1, 1) * 4;
	pthread_t tid[numThreads];
	int thread_id[numThreads];

	outputValues = (cplx*)calloc(N, sizeof(cplx));

	createThreads(tid, thread_id);
	printOutput();

	freeMemory();
	return 0;
}
