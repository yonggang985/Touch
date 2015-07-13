/* 
 * File:   algoPS.h
 */

#ifndef ALGOPS_H
#define	ALGOPS_H

#include "JoinAlgorithm.h"

class algoPS : public JoinAlgorithm {
public:
    algoPS()
    {
        algorithm = algo_PS;
    }
    ~algoPS() {}
    
    void run()
    {
        totalTimeStart();
        readBinaryInput(file_dsA, file_dsB);
        PS(dsA,dsB);
        totalTimeStop();
        clearMem = (dsA.size()+dsB.size())*(sizeof(TreeEntry*)+dsA.front()->obj->getSize());
        process_mem_usage(swapMem, ramMem);
    }
    
    void PS(SpatialObjectList& A, SpatialObjectList& B)
    {

	//Sort the datasets based on their lower x coordinate
	sorting.start();
	thrust::sort(A.begin(), A.end(), Comparator_Xaxis());
	thrust::sort(B.begin(), B.end(), Comparator_Xaxis());
	sorting.stop();

	//sweep
	FLAT::uint64 iA=0,iB=0;
	while(iA<A.size() && iB<B.size())
	{
            if(A[iA]->getMBR().low[0] < B[iB]->getMBR().low[0])
            {
                FLAT::uint64 i = iB;
                while(i<B.size() && B[i]->getMBR().low[0] <= A[iA]->getMBR().high[0])
                {
                    if (istouching(B[i] , A[iA]))
                        resultPairs.addPair( B[i] , A[iA] );
                    i++;
                }
                iA++;
            }
            else
            {
                FLAT::uint64 i = iA;
                while(i<A.size() &&  A[i]->getMBR().low[0] <= B[iB]->getMBR().high[0])
                {
                    if ( istouching(B[iB] , A[i]) )
                    {
                        resultPairs.addPair( B[i] , A[iA] );
                    }
                    i++;
                }
                iB++;
            }
	}
    }
};

#endif	/* ALGOPS_H */

