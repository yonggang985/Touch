/* 
 * File:   CommonTOUCH.cpp
 * Author: Sadegh Nobari
 * 
 */

#include "CommonTOUCH.h"

CommonTOUCH::CommonTOUCH() {
    localPartitions = 100;
    addFilter = 0;
}

CommonTOUCH::~CommonTOUCH() {
}

/*
 * ancestorNode is node where obj is assigned.
 * First ancestor node is not checked, go through children
 */
void CommonTOUCH::joinObjectToDesc(TreeEntry* obj, TreeNode* ancestorNode)
{
    queue<TreeNode*> nodes;
    nodes.push(ancestorNode);
    TreeNode* node;
    while(nodes.size()>0)
    {
        //start from checking children, each for intersection of MBR
        // then if intersects - check the assign objects of child
        // and if it is not a leaf node and intersects -> add to the queue

        node = nodes.front();
        nodes.pop();

        if (node->leafnode == true)
            continue;

        //intersect with all non-null children
        for (NodeList::iterator it = node->entries.begin(); it != node->entries.end(); it++)
        {
            
                //if intersects
                ItemsMaxCompared += (*it)->attachedObjs[!obj->type].size();
                comparing.start();
                if(localJoin == algo_SGrid && (*it)->attachedObjs[!obj->type].size() > 0)
                {
                    (*it)->spatialGridHash[!obj->type]->probe(obj);
                }
                else
                {
                    NL(obj, (*it)->attachedObjs[!obj->type]);
                }
                comparing.stop();
            
            if (FLAT::Box::overlap(obj->mbr, (*it)->mbr))
            {
                nodes.push((*it));
            } 
            else
            {
                addFilter+=(*it)->objBelow[!obj->type];
            }
        }

    }
}

void CommonTOUCH::joinNodeToDesc(TreeNode* node)
{
    /*
     * A -> B_below
     */
    for (SpatialObjectList::iterator it = node->attachedObjs[0].begin();
                                                    it != node->attachedObjs[0].end(); it++)
    {
        joinObjectToDesc((*it), node);
    }

    /*
     * B -> A_below
     */
    for (SpatialObjectList::iterator it = node->attachedObjs[1].begin();
                                                    it != node->attachedObjs[1].end(); it++)
    {
        joinObjectToDesc((*it), node);
    }

    /*
     * A -> B
     */
    
    if (node->attachedObjs[0].size() < node->attachedObjs[1].size())
    {
        if(localJoin == algo_SGrid && node->attachedObjs[0].size() > 0)
        {
            comparing.start();
            node->spatialGridHash[0]->probe(node->attachedObjs[1]);
            comparing.stop();
        }
        else
        {
            for (SpatialObjectList::iterator it = node->attachedObjs[0].begin();
                                                            it != node->attachedObjs[0].end(); it++)
            {
                ItemsMaxCompared += node->attachedObjs[1].size();                                
                comparing.start();
                    if (FLAT::Box::overlap((*it)->mbr, node->mbrSelfD[1]))
                        NL((*it), node->attachedObjs[1]);
                comparing.stop();
            }
        }
        
        
    }
    else 
    {
        if(localJoin == algo_SGrid && node->attachedObjs[1].size() > 0)
        {
            comparing.start();
            node->spatialGridHash[1]->probe(node->attachedObjs[0]);
            comparing.stop();
        }
        else
        {
            for (SpatialObjectList::iterator it = node->attachedObjs[1].begin();
                                    it != node->attachedObjs[1].end(); it++)
            {
                ItemsMaxCompared += node->attachedObjs[0].size();
                comparing.start();
                
                    if (FLAT::Box::overlap((*it)->mbr, node->mbrSelfD[0]))
                        NL((*it), node->attachedObjs[0]);
                comparing.stop();
            }
        }
    }
}

void CommonTOUCH::probe()
{
    if(localJoin == algo_SGrid && treeTraversal == join_TD && algorithm != algo_TOUCH)
        countSpatialGrid();
    
    
    probing.start();
    
    switch (treeTraversal)
    {
        case join_BU:
            probeDownUp();
            break;
        case join_TDD:
            probeUpDown();
            //NOT SUPPORTED
            break;
        case join_TDF:
            probeUpDownFilter();
            break;
        case join_TD:
            std::queue<TreeNode*> Qnodes;
            TreeNode* currentNode;
            Qnodes.push(root);

            int lvl = Levels;
            // A BFS on the tree then for each find all its leaf nodes by another BFS
            while(Qnodes.size()>0)
            {
                currentNode = Qnodes.front();
                Qnodes.pop();
                //BFS on the tree using Qnodes as the queue for memorizing the future nodes to be traversed
                if(!currentNode->leafnode)
                    for (NodeList::iterator it = currentNode->entries.begin(); it != currentNode->entries.end(); it++)
                    {
                            Qnodes.push((*it));
                    }

                // just to display the level of the BFS traversal
                if (verbose)
                    if(lvl!=currentNode->level)
                    {
                            lvl = currentNode->level;
                            cout << "\n### Level " << lvl <<endl;
                    }

                // If the current node has no objects assigned to it, no join is needed for the current node to the leaf nodes.

                if(currentNode->attachedObjs[0].size() + currentNode->attachedObjs[1].size()
                        + currentNode->attachedObjsAns[0].size() + currentNode->attachedObjsAns[1].size() ==0)
                    continue;
                joinNodeToDesc(currentNode);
            }   
            break;
    }
    probing.stop();
    
//        clearMem = 0; //memory footprint
//        
//    if (localJoin == algo_SGrid) {
//        for (NodeList::iterator it = tree.begin(); it != tree.end(); it++)
//        {
//            clearMem += getMemFootprint((*it));
//        }
//        clearMem += sizeof(TreeNode*)*(tree.size()+1);
//    }
        
    if(localJoin == algo_SGrid && treeTraversal == join_TD && algorithm != algo_TOUCH)
    {
        
        deduplicateSpatialGrid();
    }
}

void CommonTOUCH::JOIN(TreeNode* node, TreeNode* nodeObj)
{
    int type;
    if (node == nodeObj)
    {
        
		if (node->attachedObjs[0].size()+node->attachedObjsAns[0].size() < 
				node->attachedObjs[1].size() + node->attachedObjsAns[1].size())
		{
			type = 0;
		}
		else 
		{
			type = 1;
		}
        
        if (localJoin == algo_SGrid)
        {
            if (node->attachedObjs[type].size() > 0)
            {
                node->spatialGridHash[type]->probe(nodeObj->attachedObjs[!type]);
                node->spatialGridHash[type]->probe(nodeObj->attachedObjsAns[!type]);
            }
            if (node->attachedObjsAns[type].size() > 0)
            {
                node->spatialGridHashAns[type]->probe(nodeObj->attachedObjs[!type]);
                node->spatialGridHashAns[type]->probe(nodeObj->attachedObjsAns[!type]);
            }
        }
        else
        {
            NL(node->attachedObjs[type],nodeObj->attachedObjs[!type]);
            NL(node->attachedObjsAns[type],nodeObj->attachedObjs[!type]);
            NL(node->attachedObjs[type],nodeObj->attachedObjsAns[!type]);
            NL(node->attachedObjsAns[type],nodeObj->attachedObjsAns[!type]);
        }
        
        ItemsMaxCompared += (node->attachedObjs[type].size()+node->attachedObjsAns[type].size())*
                            (nodeObj->attachedObjs[!type].size() + nodeObj->attachedObjsAns[!type].size());
        
        return;
    }
    for (int type = 0; type < TYPES; type++)
    {
        
        if (localJoin == algo_SGrid)
        {
            if (node->attachedObjs[type].size() > 0) node->spatialGridHash[type]->probe(nodeObj->attachedObjs[!type]);
            if (node->attachedObjsAns[type].size() > 0) node->spatialGridHashAns[type]->probe(nodeObj->attachedObjs[!type]);
        }
        else
        {
            NL(node->attachedObjs[type],nodeObj->attachedObjs[!type]);
            NL(node->attachedObjsAns[type],nodeObj->attachedObjs[!type]);
        }
        ItemsMaxCompared += (node->attachedObjs[type].size()+node->attachedObjsAns[type].size())*
                            (nodeObj->attachedObjs[!type].size());
    }
}

void CommonTOUCH::JOINdown(TreeNode* node, TreeNode* nodeObj)
{
    int type;
    if (node == nodeObj)
    {
        if (node->attachedObjs[0].size()+node->attachedObjsAns[0].size() < 
                node->attachedObjs[1].size() + node->attachedObjsAns[1].size())
        {
            type = 0;;
        }
        else 
        {
            type = 1;
        }
        
        if (localJoin == algo_SGrid)
        {
            node->spatialGridHash[type]->probe(nodeObj->attachedObjs[!type]);
            node->spatialGridHashAns[type]->probe(nodeObj->attachedObjs[!type]);
            node->spatialGridHash[type]->probe(nodeObj->attachedObjsAns[!type]);
            node->spatialGridHashAns[type]->probe(nodeObj->attachedObjsAns[!type]);
        }
        else
        {
            NL(node->attachedObjs[type],nodeObj->attachedObjs[!type]);
            NL(node->attachedObjsAns[type],nodeObj->attachedObjs[!type]);
            NL(node->attachedObjs[type],nodeObj->attachedObjsAns[!type]);
            NL(node->attachedObjsAns[type],nodeObj->attachedObjsAns[!type]);
        }
        
        ItemsMaxCompared += (node->attachedObjs[type].size()+node->attachedObjsAns[type].size())*
                            (nodeObj->attachedObjs[!type].size() + nodeObj->attachedObjsAns[!type].size());
        
        return;
    }
    for (int type = 0; type < TYPES; type++)
    {
        
        if (localJoin == algo_SGrid)
        {
            node->spatialGridHash[type]->probe(nodeObj->attachedObjs[!type]);
            node->spatialGridHash[type]->probe(nodeObj->attachedObjsAns[!type]);
        }
        else
        {
            NL(node->attachedObjs[type],nodeObj->attachedObjs[!type]);
            NL(node->attachedObjs[type],nodeObj->attachedObjsAns[!type]);
        }
        ItemsMaxCompared += (node->attachedObjs[type].size())*
                            (nodeObj->attachedObjs[!type].size()+nodeObj->attachedObjsAns[!type].size());
    }
}

void CommonTOUCH::pathWayJoin(TreeNode* node)
{
    probingList.push_back(node);
    for (NodeList::iterator cit = node->entries.begin(); cit != node->entries.end(); cit++)
    {
        pathWayJoin((*cit));
    }
   
    if (localJoin == algo_SGrid) countSpatialGrid(node);
    
    for (NodeList::iterator ancit = probingList.begin(); ancit != probingList.end(); ancit++)
    {
        JOIN(node,(*ancit));
    }
    if (localJoin == algo_SGrid) deduplicateSpatialGrid(node);
    probingList.pop_back();
}

void CommonTOUCH::JoinDownR(TreeNode* node, TreeNode* nodeObj)
{
    JOINdown(node, nodeObj);
    for (NodeList::iterator entr = nodeObj->entries.begin();
            entr != nodeObj->entries.end(); entr++)
    {
        JoinDownR(node, (*entr));
    }
}

void CommonTOUCH::JoinDownRFilter(TreeNode* node, TreeNode* nodeObj)
{
    JOINdown(node, nodeObj);
    for (NodeList::iterator entr = nodeObj->entries.begin();
            entr != nodeObj->entries.end(); entr++)
    {
        if (FLAT::Box::overlap((*entr)->mbr,node->mbr))
        {
            JoinDownRFilter(node, (*entr));
        }
    }
}

void CommonTOUCH::pathWayJoinDown(TreeNode* node)
{
    if (localJoin == algo_SGrid) countSpatialGrid(node);
    JoinDownR(node, node);
    if (localJoin == algo_SGrid) deduplicateSpatialGrid(node);
    
    for (NodeList::iterator cit = node->entries.begin(); cit != node->entries.end(); cit++)
    {
        pathWayJoinDown((*cit));
    }
}

void CommonTOUCH::pathWayJoinDownFilter(TreeNode* node)
{
    if (localJoin == algo_SGrid) countSpatialGrid(node);
    JoinDownRFilter(node, node);
    if (localJoin == algo_SGrid) deduplicateSpatialGrid(node);
    
    for (NodeList::iterator cit = node->entries.begin(); cit != node->entries.end(); cit++)
    {
        pathWayJoinDownFilter((*cit));
    }
}

void CommonTOUCH::probeDownUp()
{
    pathWayJoin(root);
}

void CommonTOUCH::probeUpDown()
{
    pathWayJoinDown(root);
}

void CommonTOUCH::probeUpDownFilter()
{
    pathWayJoinDownFilter(root);
}

void CommonTOUCH::countSizeStatistics()
{
    //put Ans and not Ans together!
    
    sizeCalculate.start();
    FLAT::Box mbr;
    FLAT::Vertex vec;
    double v;
    for (NodeList::iterator it = tree.begin(); it != tree.end(); it++)
    { 
        for (int type = 0; type < TYPES; type++)
        {
            for (int dim = 0; dim < DIMENSION; dim++)
            {
                (*it)->avrSize[type][dim] = 0;
                (*it)->stdSize[type][dim] = 0;
            }
            (*it)->avrVol[type] = 0;
            
            
            for (SpatialObjectList::iterator oBit = (*it)->attachedObjs[type].begin();
                    oBit != (*it)->attachedObjs[type].end(); oBit++)
            {
                FLAT::Vertex::differenceVector((*oBit)->getMBR().high,(*oBit)->getMBR().low,vec);
                v = FLAT::Box::volume((*oBit)->getMBR());
                for (int dim = 0; dim < DIMENSION; dim++)
                {
                    (*it)->avrSize[type][dim] += vec[dim];
                    (*it)->stdSize[type][dim] += vec[dim]*vec[dim];
                }
                (*it)->avrVol[type] += v;
            }
            for (SpatialObjectList::iterator oBit = (*it)->attachedObjsAns[type].begin();
                    oBit != (*it)->attachedObjsAns[type].end(); oBit++)
            {
                FLAT::Vertex::differenceVector((*oBit)->getMBR().high,(*oBit)->getMBR().low,vec);
                v = FLAT::Box::volume((*oBit)->getMBR());
                for (int dim = 0; dim < DIMENSION; dim++)
                {
                    (*it)->avrSize[type][dim] += vec[dim];
                    (*it)->stdSize[type][dim] += vec[dim]*vec[dim];
                }
                (*it)->avrVol[type] += v;
            }
        }
    }
    sizeCalculate.stop();
}

void CommonTOUCH::countSpatialGrid()
{
    for (NodeList::iterator it = tree.begin(); it != tree.end(); it++)
    { 
        countSpatialGrid((*it));
    }
}

void CommonTOUCH::countSpatialGrid(TreeNode* node)
{   
    gridCalculate.start();
    FLAT::Box mbr;
    double resolution;
    double resolution3d[DIMENSION];
    for (int type = 0; type < TYPES; type++)
    {
        
        mbr = node->mbrSelfD[type];
        
        switch (SGResol)
        {
            case Static_SG_Resolution:
                
                node->spatialGridHash[type] = new SpatialGridHash();
                node->spatialGridHash[type]->init(mbr,localPartitions);
                node->spatialGridHash[type]->epsilon = this->epsilon;
                node->spatialGridHash[type]->build(node->attachedObjs[type]);
                
                break;
            case Dynamic_Equal_SG_Resolution:
                for (int dim = 0; dim < DIMENSION; dim++)
                {
                    if (node->avrVol[type] == 0)
                    {
                        resolution = 1; // no objects
                    }
                    else
                    {
                        resolution = (double) std::pow(node->avrVol[type]/(node->attachedObjs[type].size()+
                                node->attachedObjsAns[type].size()),1./3.);
                    }
                }
                
                node->spatialGridHash[type] = new LocalSpatialGridHash();
                node->spatialGridHash[type]->init(mbr,resolution);
                node->spatialGridHash[type]->epsilon = this->epsilon;
                node->spatialGridHash[type]->build(node->attachedObjs[type]);
             
                break;
            case Dynamic_Flex_SG_Resolution:
                
                FLAT::Vertex spaceVec;
                FLAT::Vertex::differenceVector(mbr.high,mbr.low,spaceVec);

                for (int dim = 0; dim < DIMENSION; dim++)
                {
                    if (node->avrSize[type][dim] == 0)
                    {
                        resolution3d[dim] = 1; // no objects
                    }
                    else
                    {
                        resolution3d[dim] = (double) node->avrSize[type][dim]/(node->attachedObjs[type].size()+
                                node->attachedObjsAns[type].size());
                    }
                }
                
                
                node->spatialGridHash[type] = new FlexLocalSpatialGridHash();
                node->spatialGridHash[type]->init(mbr,resolution3d[0],resolution3d[1],resolution3d[2]);
                node->spatialGridHash[type]->epsilon = this->epsilon;
                node->spatialGridHash[type]->build(node->attachedObjs[type]);
                
                break;
        }

    }
    gridCalculate.stop();
}

void CommonTOUCH::deduplicateSpatialGrid()
{
    for (NodeList::iterator it = tree.begin(); it != tree.end(); it++)
    {
        deduplicateSpatialGrid((*it));
    }
    
    //@todo free memory
}

void CommonTOUCH::deduplicateSpatialGrid(TreeNode* node)
{
    for (int type = 0; type < TYPES; type++)
    {
        node->spatialGridHash[type]->resultPairs.deDuplicate();
        
        SpatialGridHash::transferInfo(node->spatialGridHash[type], this);
   
    }
}

void CommonTOUCH::writeNode(SpatialObjectList& objlist)
{
    TreeNode* prNode = new TreeNode(0);
    FLAT::Box mbr;
    totalnodes++;
    
    for (SpatialObjectList::iterator it=objlist.begin(); it!=objlist.end(); ++it)
    {
        prNode->attachedObjs[(*it)->type].push_back(*it);
        mbr = FLAT::Box::combineSafe((*it)->mbr,mbr);
    }
    prNode->mbr = mbr;
    prNode->mbrL[0] = mbr;
    prNode->mbrL[1] = mbr;
    prNode->id = tree.size();
    tree.push_back(prNode);
    nextInput.push_back(prNode);
}

void CommonTOUCH::writeNode(NodeList& nodelist, int Level)
{
    TreeNode* prNode = new TreeNode(Level);
    FLAT::Box mbr;
    totalnodes++;
    
    for (NodeList::iterator it=nodelist.begin(); it!=nodelist.end(); ++it)
    {
            prNode->entries.push_back(*it);
            mbr = FLAT::Box::combineSafe((*it)->mbr,mbr);
    }
    prNode->mbr = mbr;
    prNode->mbrL[0] = mbr;
    prNode->mbrL[1] = mbr;
    prNode->id = tree.size();
    tree.push_back(prNode);
    nextInput.push_back(prNode);
}

void CommonTOUCH::createPartitions(SpatialObjectList& vds)
{
    partition.start();

    Levels = 1;
    totalnodes = 0;
    createTreeLevel(vds);
    if (verbose) std::cout << "Tree leafs sorted." << std::endl;
    NodeList nds;
    swap(nds,nextInput);
    nextInput.clear();
    while(nds.size()>1)
    {
        if (verbose) 
            std::cout << "Tree Level: " << Levels << " Tree Nodes: " << tree.size() 
                      << " Remaining Input: " << nds.size() <<endl;
            createTreeLevel(nds,Levels++);      // writes final nodes in tree and next level in nextInput
            swap(nds,nextInput);					// swap input and nextInput list
            nextInput.clear();
    }
    
    root = nds.front();
    root->root = true;
    if (verbose) std::cout << "Levels " << Levels << std::endl;
    partition.stop();
}

void CommonTOUCH::createTreeLevel(SpatialObjectList& input)
{
    sorting.start();
//    for (int i = 0; i < input.size(); i++)
//    {
//        cout << "good " << input[i]->id << endl;
//    }
//    cout << " endl " << endl;
    switch (PartitioningType)
    {
        case Hilbert_Sort:
            thrust::sort(input.begin(),input.end(),ComparatorHilbertEntry());
            break;
        case No_Sort:
            break;
        default:
            thrust::sort(input.begin(),input.end(),ComparatorEntry());
            break;
    }
    sorting.stop();

    if (verbose) std::cout << "Sort "<< input.size()<< " leaf objects in " << sorting << std::endl;
    
    SpatialObjectList entries;
    for (SpatialObjectList::iterator it=input.begin();it!=input.end();++it)
    {
            if (entries.size()<leafsize)
            {
                    entries.push_back(*it);
                    if (entries.size()>=leafsize)
                    {
                            writeNode(entries);
                            entries.clear();
                    }
            }
    }
    if (!entries.empty())
            writeNode(entries);
}

void CommonTOUCH::createTreeLevel(NodeList& input, int Level)
{
    
    sorting.start();
    switch (PartitioningType)
    {
        case Hilbert_Sort:
            thrust::sort(input.begin(),input.end(),ComparatorHilbert());
            break;
        case No_Sort:
            break;
        default:
            thrust::sort(input.begin(),input.end(),Comparator());
            break;
    }
    sorting.stop();

    if (verbose) std::cout << "Sort "<< input.size()<< " items in " << sorting << std::endl;
    
    NodeList entries;
    for (NodeList::iterator it=input.begin();it!=input.end();++it)
    {
            if (entries.size()<nodesize)
            {
                    entries.push_back(*it);
                    if (entries.size()>=nodesize)
                    {
                            writeNode(entries,Level);
                            entries.clear();
                    }
            }
    }
    if (!entries.empty())
            writeNode(entries,Level);
}



void CommonTOUCH::analyze()
{
    countSizeStatistics(); // must be before analysis to count average sizes

    analyzing.start();
    FLAT::uint64 emptyCells;
    FLAT::uint64 sum=0, sqsum=0;
    double differenceSquared=0;
    //footprint += vdsA.size()*(sizeof(TreeEntry*));
    //footprint += dsB.size()*(sizeof(FLAT::SpatialObject*));
    FLAT::uint64 cursum;
    
    for (int type = 0; type < TYPES; type++)
    {
        ItemPerLevel[type].resize(Levels,0);
        ItemPerLevelAns[type].resize(Levels,0);
        emptyCells = 0;
        
        for(NodeList::iterator ni = tree.begin(); ni != tree.end(); ni++)
        {
                cursum = (*ni)->attachedObjs[type].size() + (*ni)->attachedObjsAns[type].size();
                if (cursum==0) emptyCells++;
                ItemPerLevel[type][(*ni)->level] += (*ni)->attachedObjs[type].size();
                ItemPerLevelAns[type][(*ni)->level] += (*ni)->attachedObjsAns[type].size();
                sum += cursum;
                sqsum += cursum*cursum;
                if (maxMappedObjects < cursum) maxMappedObjects = cursum; // save maximum assigned objects
                if ((*ni)->level < 10)
                {
                    levelAvg[type][(*ni)->level] += ((*ni)->avrSize[type][0]+(*ni)->avrSize[type][1]+(*ni)->avrSize[type][2])/3.;
                    levelStd[type][(*ni)->level] += ((*ni)->stdSize[type][0]+(*ni)->stdSize[type][1]+(*ni)->stdSize[type][2])/3.; //not right, but just for visibility
                }
        }


        int top10Level = (Levels>10)?10:Levels;
        for(int i = 0 ; i<top10Level ; i++)
        {
            levelAssigned[type][i] = ItemPerLevel[type][i]+ItemPerLevelAns[type][i];
            if (levelAssigned[type][i] != 0)
            {
                levelAvg[type][i] /= (double)levelAssigned[type][i];
                levelStd[type][i] = sqrt(levelStd[type][i]/(double)levelAssigned[type][i] - levelAvg[type][i]*levelAvg[type][i]);
            }
        }
        
        if (verbose)
            for(int i = 0 ; i<Levels; i++)
                std::cout<< "Type " << type << " Level " << i << ": Items " << ItemPerLevel[type][i] 
                        <<  " Ans " << ItemPerLevelAns[type][i] << " Avr size " << levelAvg[type][i]
                        << " Std " << levelStd[type][i] << endl;
    }
    
    /*
     * Size statistics counts for each type and each cell. So, there is total
     * 2*tree.size cells with objects (for each type)
     */
    
    //footprint += sum*sizeof(FLAT::SpatialObject*) + tree.size()*(sizeof(TreeNode*));
    avg = (sum+0.0) / (TYPES*tree.size());
    percentageEmpty = (emptyCells+0.0) / (TYPES*tree.size())*100.0;
    differenceSquared = ((double)sqsum/(TYPES*(double)tree.size()))-avg*avg;
    std = sqrt(differenceSquared);
    
    
    process_mem_usage(swapMem, ramMem);
    countObjBelowStart();
    
    
    analyzing.stop();

}

unsigned int CommonTOUCH::countObjBelow(TreeNode* node, int type)
{
    FLAT::uint64 res = 0;
    node->objBelow[type]=0;
    for (NodeList::iterator it = node->entries.begin(); it != node->entries.end(); it++)
    {
        node->objBelow[type] += countObjBelow((*it),type);
        //res += node->objBelow[type];
    }
    res += node->attachedObjs[type].size();
    res += node->attachedObjsAns[type].size();
    return node->objBelow[type]+res;
}

void CommonTOUCH::countObjBelowStart()
{
    for (int i = 0; i < TYPES; i++)
    {
        countObjBelow(root, i);
    }
}