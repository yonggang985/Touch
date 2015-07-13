/* 
 * File:   TreeNode.h
 *
 */

#ifndef TREENODE_H
#define	TREENODE_H

#include "TreeEntry.h"
#include "ResultPairs.h"

class LocalSpatialGridHash;
class SpatialGridHash;

class TreeNode
{
public:
    unsigned int id;
    
    	FLAT::Box mbrK[TYPES]; //combined black and light MBR of each type
        FLAT::uint64 num[TYPES];// number of objects assigned below

	FLAT::Box mbrSelfD[TYPES]; //mbr's of objects which were assigned to current node
	
	FLAT::Box mbrL[TYPES]; //light mbr
	FLAT::Box mbrD[TYPES]; //dark mbr
        
        FLAT::Box mbr; //not used
    
	thrust::host_vector<TreeNode*> entries;
        
        TreeNode* parentNode;
        
	bool leafnode;
        bool root;
	int level;
        SpatialGridHash* spatialGridHash[TYPES];   
        SpatialGridHash* spatialGridHashAns[TYPES];
	
	thrust::host_vector<TreeEntry*> attachedObjs[TYPES];
        thrust::host_vector<TreeEntry*> attachedObjsAns[TYPES];
        
        double avrSize[TYPES][DIMENSION];
        double avrVol[TYPES];
        double stdSize[TYPES][DIMENSION];
        
        FLAT::uint64 objBelow[TYPES];
    
	TreeNode(int Level)
	{
		level = Level;
                root = 0;
		if (Level==0) leafnode = true;
		else leafnode = false;
                mbr.isEmpty = true;
	}
	
	TreeNode(const FLAT::Box& MbrA, const FLAT::Box& MbrB)
	{
		mbrL[1] = MbrB;
		mbrL[0] = MbrA;
                mbrK[0] = MbrA;
                mbrK[1] = MbrB;
                mbr = FLAT::Box::combineSafe(MbrA,MbrB);
	}
};



#endif	/* TREENODE_H */

