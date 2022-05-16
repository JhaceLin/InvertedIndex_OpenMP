#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>
#include <bitset>
#include "config.h"
#include <omp.h>
#include <windows.h>
#include <algorithm>
using namespace std;

const int INF = 1 << 30;
const int NUM_THREADS = 4;
const bool InParallel = true;

int numQuery = 0;
int query[10];
bool EndReadQuery = false;
ifstream queryFile, indexFile;
int numIndex = 0;
int lenIndex[maxNumIndex];
int iindex[maxNumIndex][maxLenIndex];

bitset <32> **bindex;
int *len_bindex;

bitset <32> *rst;
int *result;
int resultCount;
int rstLen;

int max_len_bindex = -1;

class MyTimer {
    private:
        long long start_time, finish_time, freq;
        double timeMS;

    public:
        MyTimer() {
            timeMS = 0;
            QueryPerformanceFrequency((LARGE_INTEGER*) &freq);
        }

        void start() {
            QueryPerformanceCounter((LARGE_INTEGER*) &start_time);
        }

        void finish() {
            QueryPerformanceCounter((LARGE_INTEGER*) &finish_time);
        }
        
        double getTime() {
            // ms
            double duration = (finish_time - start_time) * 1000.0 / freq;
            return duration;
        }
};

bool ReadIndex()
{
    if (!indexFile)
    {
        cout << "Error opening file.";
        return false;
    }

    numIndex = 0;
    int num = 0;
    while (indexFile.read(reinterpret_cast<char *>(&num), sizeof(int)))
    {
        lenIndex[numIndex] = num;
        indexFile.read(reinterpret_cast<char *>(iindex[numIndex]), sizeof(int) * num);
        numIndex++;
    }

    len_bindex = new int[numIndex];
    bindex = new bitset<32> *[numIndex];

    int bstNum;
    #pragma omp parallel for if(InParallel), num_threads(NUM_THREADS), private(bstNum)
    for(int i = 0; i < numIndex; i++) {
        bstNum = iindex[i][lenIndex[i] - 1];
        bstNum /= 32;
        bstNum++;
        len_bindex[i] = bstNum;

        #pragma omp critical(max_len_bindex)
        {
            if(bstNum > max_len_bindex) {
                max_len_bindex = bstNum;
            }
        }

        bindex[i] = new bitset<32>[bstNum];
        for(int j = 0; j < lenIndex[i]; j++) {
            int t = iindex[i][j];
            bindex[i][t / 32].set(t % 32);
        }
    }

    return true;
}

bool ReadQuery()
{ // Read A Row At a Time
    if (!queryFile)
    {
        cout << "Error opening file.";
        return false;
    }

    numQuery = 0;
    int x;
    string list;
    if (getline(queryFile, list))
    {
        istringstream islist(list);
        while (islist >> x)
        {
            query[numQuery++] = x;
        }
        return true;
    }
    else
    {
        return false;
    }
}

void ReleaseSpace() {
    // Delete bindex
    for(int i = 0; i < numIndex; i++) {
        delete[] bindex[i];
    }
    delete[] bindex;

    // Delete len_bindex
    delete[] len_bindex;

    // Delete result
    delete[] result;

    // Delete rst
    delete[] rst;
}

void GetIntersectionAndAnswer() {
    if(numQuery < 2) return;

    #pragma omp master
    {
        rstLen = INF;
        for(int i = 0; i < numQuery; i++) {
            int t = query[i];
            if(len_bindex[t] < rstLen) {
                rstLen = len_bindex[t];
            }
        }
    }
    #pragma omp barrier

    #pragma omp for
    for(int sect = 0; sect < rstLen; sect++) {
        // Get Intersection
        rst[sect] = bindex[query[0]][sect];
        for(int i = 1; i < numQuery; i++) {
            rst[sect] &= bindex[query[i]][sect];
        }

        // Get Answer
        if(!rst[sect].none()) {
            for(int i = 0; i < 32; i++) {
                if(rst[sect].test(i)) {
                    int x = sect * 32 + i;

                    #pragma omp critical(resultCount)
                    result[resultCount++] = x;
                }
            }
        }
    }    
}

void ShowResult() {
    cout << resultCount << ": ";

    sort(result, result + resultCount);

    for(int i = 0; i < resultCount; i++) {
        cout << result[i] << " ";
    }
    cout << endl;
}

void test_code() {
    for(int i = 0; i < numIndex; i++) {
        for(int j = 0; j < len_bindex[i]; j++) {
            cout << bindex[i][j] << " ";
        }
        cout << endl;
    }
}

int main() {
    bool testMode = false;

    MyTimer fileInTimer, computeTimer;

    fileInTimer.start();
    if(testMode) {
        indexFile.open(testIndexEnlargePath, ios::in | ios::binary);
        queryFile.open(testQueryPath, ios ::in);
    }
    else {
        queryFile.open(queryPath, ios ::in);
        indexFile.open(indexPath, ios::in | ios::binary);
    }

    if (!ReadIndex())
    {
        cout << "Error: Fail to reading index" << endl;
        return 0;
    }

    fileInTimer.finish();
    printf("Time Used in File Reading: %8.4fms\n", fileInTimer.getTime());

    computeTimer.start();

    rst = new bitset <32> [max_len_bindex];
    result = new int[maxLenIndex];
    resultCount = 0;
    int readCount = 0;

    EndReadQuery = false;
    ReadQuery();
    #pragma omp parallel if(InParallel), num_threads(NUM_THREADS)
    {
        while(!EndReadQuery) {
            GetIntersectionAndAnswer();

            #pragma omp master
            {
                if(testMode) {
                    cout << "Result: " << endl;
                    ShowResult();
                }
                resultCount = 0;

                if(!ReadQuery()) {
                    EndReadQuery = true;
                }
            }
            #pragma omp barrier
        }
    }
    
    computeTimer.finish();
    printf("Time Used in Computing Intersection: %8.4fms\n", computeTimer.getTime());

    ReleaseSpace();
    queryFile.close();
    indexFile.close();

    cout << "End" << endl;
}