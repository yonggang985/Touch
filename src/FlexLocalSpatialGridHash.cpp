/* 
 * File:   FlexLocalSpatialGridHash.cpp
 */

#include "FlexLocalSpatialGridHash.h"

void FlexLocalSpatialGridHash::init(const FLAT::Box& universeExtent,const double gridResolutionPerDimension0,
        const double gridResolutionPerDimension1,const double gridResolutionPerDimension2)	
{
        initialize.start();
        /*
         * universeWidth - number of cells per dimension
         * resolution - size of square cell
         */
        universe = universeExtent;
        resolution[0] = gridResolutionPerDimension0;
        resolution[1] = gridResolutionPerDimension1;
        resolution[2] = gridResolutionPerDimension2;
        FLAT::Vertex difference;
        FLAT::Vertex::differenceVector(universeExtent.high,universeExtent.low,difference);
        localPartitions=1;
        for (int i=0;i<DIMENSION;++i)
        {
                universeWidth[i] = ceil(difference[i]/resolution[i]);
                //if (universeWidth[i] > 50)                    universeWidth[i] = 50;
                
                //if (resolution[i] != 1) cout << "Number of cells: " << universeWidth[i] << " Universe width:" << difference[i] << " Resolution:" << resolution[i] << " ;;; \n";
                localPartitions *= universeWidth[i];
        }
        initialize.stop();
}

FlexLocalSpatialGridHash::~FlexLocalSpatialGridHash() {
    for (HashTable::iterator it = gridHashTable.begin(); it!=gridHashTable.end(); ++it)
            delete it->second;
}

void FlexLocalSpatialGridHash::analyze(const SpatialObjectList& dsA,const SpatialObjectList& dsB)
{
    cout << "Wrong analyze function (flex)" << endl;
        analyzing.start();
        footprint += dsA.capacity()*(sizeof(TreeEntry*));
        footprint += dsB.capacity()*(sizeof(TreeEntry*));
        if (verbose) cout << "Cell Width Local Flex SGrid: " << universeWidth  << endl;
        
        FLAT::uint64 sum=0,sqsum=0;
        for (HashTable::iterator it = gridHashTable.begin(); it!=gridHashTable.end(); ++it)
        {
                FLAT::uint64 ptrs=((SpatialObjectList*)(it->second))->size();
                sum += ptrs;
                sqsum += ptrs*ptrs;
                if (maxMappedObjects<ptrs) maxMappedObjects = ptrs;
        }
        footprint += sum*sizeof(TreeEntry*) +  sizeof(HashValue)*localPartitions;
        avg = (sum+0.0) / (localPartitions+0.0);
        percentageEmpty = (double)(localPartitions - gridHashTable.size()) / (double)(localPartitions)*100.0;
        repA = (double)(sum)/(double)size_dsA;
        double differenceSquared=0;
        differenceSquared = ((double)sqsum/(double)localPartitions)-avg*avg;
        std = sqrt(differenceSquared);
        analyzing.stop();
}

void FlexLocalSpatialGridHash::build(SpatialObjectList& dsA)
{
        building.start();
        gridHashTable.clear();
        for(SpatialObjectList::iterator i=dsA.begin(); i!=dsA.end(); ++i)
        {
                vector<FLAT::uint64> cells;
                getOverlappingCells(*i,cells);
                for (vector<FLAT::uint64>::iterator j = cells.begin(); j!=cells.end(); ++j)
                {
                        HashTable::iterator it= gridHashTable.find(*j);
                        if (it==gridHashTable.end())
                        {
                                HashValue* soList = new HashValue();
                                soList->push_back(*i);
                                gridHashTable.insert( ValuePair(*j,soList) );
                        }
                        else
                        {
                                it->second->push_back(*i);
                        }
                }
        }
        building.stop();
}

void FlexLocalSpatialGridHash::clear()
{
        gridHashTable.clear();
        //for (HashTable::iterator it = gridHashTable.begin(); it!=gridHashTable.end(); ++it)
        // delete	it->second;
}

void FlexLocalSpatialGridHash::probe(TreeEntry*& obj)
{
        probing.start();
        
        vector<FLAT::uint64> cells;
        if (!getProjectedCells( obj , cells ))
        {
                filtered[obj->type]++;
                return;
        }
        ///// Get Unique Objects from Grid Hash in Vicinity
        hashprobe += cells.size();
        for (vector<FLAT::uint64>::const_iterator j = cells.begin(); j!=cells.end(); ++j)
        {
                HashTable::iterator it = gridHashTable.find(*j);
                if (it==gridHashTable.end()) continue;
                HashValue* soList = it->second;
                for (HashValue::const_iterator k=soList->begin(); k!=soList->end(); ++k)
                        if ( istouching( obj , *k) )
                        {
                            resultPairs.addPair(obj , *k);
                        }
        }

        probing.stop();
}


void FlexLocalSpatialGridHash::probe(const SpatialObjectList& dsB)
{
    probing.start();

    for(SpatialObjectList::const_iterator i=dsB.begin(); i!=dsB.end(); ++i)
    {
        vector<FLAT::uint64> cells;
        if (!getProjectedCells( *i , cells ))
        {
            filtered[(*i)->type]++;
            continue;
        }
        ///// Get Unique Objects from Grid Hash in Vicinity
        hashprobe += cells.size();

        for (vector<FLAT::uint64>::const_iterator j = cells.begin(); j!=cells.end(); ++j)
        {
            HashTable::iterator it = gridHashTable.find(*j);
            if (it==gridHashTable.end()) continue;
            HashValue* soList = it->second;
            for (HashValue::const_iterator k=soList->begin(); k!=soList->end(); ++k)
            {
                if ( istouching( *i , *k) )
                {
                    resultPairs.addPair(*i , *k);
                }
            }
        }
    }

    probing.stop();
}