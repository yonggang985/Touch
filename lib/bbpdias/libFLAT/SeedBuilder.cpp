#include "SeedBuilder.hpp"


namespace FLAT
{
	nodeSkeleton::nodeSkeleton()
	{
		m_pIdentifier=NULL;
		m_ptrMBR=NULL;
		m_pDataLength=NULL;
		m_pData=NULL;
		level=0;
		children=0;
		nodeType=0;
		dataLength=0;
	}

	nodeSkeleton::~nodeSkeleton()
	{
		if (m_ptrMBR!=NULL)
		{
			for (uint32_t i=0;i<this->children;i++)
				if (m_ptrMBR[i]!=NULL)
					delete m_ptrMBR[i];
			delete[] m_ptrMBR;
		}

		if (m_pData != NULL)
		{
			for (uint32_t i=0;i<this->children;i++)
				if (m_pData[i]!=NULL)
					delete[] m_pData[i];
			delete[] m_pData;
		}
		if (m_pDataLength!=NULL)
			delete[] m_pDataLength;
		if (m_pIdentifier!=NULL)
			delete[] m_pIdentifier;
	}

	data_stream::data_stream (vector<MetadataEntry*>& metadataStructure)
	{
		currentMD=0;
		md = metadataStructure;
	}

	data_stream::~data_stream()
	{
	}

	bool data_stream::hasNext()
	{
		return (currentMD < md.size());
	}

	uint32_t data_stream::size()
	{
		abort();
	}

	void data_stream::rewind()
	{
		abort();
	}

	SpatialIndex::IData* data_stream::getNext()
	{
		MetadataEntry* me = md.at(currentMD);

		me->pageId = currentMD;
	    byte * buffer;
	    int length;
	    me->serialize(&buffer, length);

	    double low[3], high[3];

		low[0] = me->pageMbr.low[0];
		low[1] = me->pageMbr.low[1];
		low[2] = me->pageMbr.low[2];
		high[0] = me->pageMbr.high[0];
		high[1] = me->pageMbr.high[1];
		high[2] = me->pageMbr.high[2];

		SpatialIndex::Region r = SpatialIndex::Region(low, high, 3);

		SpatialIndex::RTree::Data* ret = new SpatialIndex::RTree::Data(length, buffer, r, currentMD); // we are puuting CurrentMD at node ids

		currentMD++;

		delete[] buffer;

		// delete Metadata from memory
		return ret;
	}

	void SeedBuilder::buildSeedTree(std::string indexFileStem, SpatialIndex::IDataStream* stream)
	{
		string seedFile = indexFileStem+"_index";
		SpatialIndex::IStorageManager* rtreeStorageManager = SpatialIndex::StorageManager::createNewDiskStorageManager(seedFile, 4096);

		SpatialIndex::id_type indexIdentifier=1;
		//data_stream* ds = new data_stream(metadataStructure);
		SpatialIndex::ISpatialIndex *seedTree = SpatialIndex::RTree::createAndBulkLoadNewRTree (
		        SpatialIndex::RTree::BLM_STR,
		        *stream,
		        *rtreeStorageManager,
		        0.7F, 20,
		        37, DIMENSION,
		        SpatialIndex::RTree::RV_RSTAR,
		        indexIdentifier);

		//delete ds;
		delete seedTree;

		vector<SpatialIndex::id_type> keys;

		// Get all the Node keys in vector .. in memory!
		rtreeStorageManager->getKeys(&keys);
		map<uint32_t,SpatialIndex::id_type> mapping;

		// Scan all the keys and build the mapping for metadata pageId and Rtree pageId
		for (vector<SpatialIndex::id_type>::iterator iter = keys.begin(); iter != keys.end(); ++iter)
		{
			SpatialIndex::id_type id = *iter;

			nodeSkeleton * nss = readNode(id, rtreeStorageManager);
			if (nss != NULL)
				if (nss->nodeType == SpatialIndex::RTree::PersistentLeaf)
					for (unsigned i = 0; i < nss->children; i++)
					{
						MetadataEntry m = MetadataEntry(nss->m_pData[i], nss->m_pDataLength[i]);

						mapping.insert (pair<uint32_t,SpatialIndex::id_type>(m.pageId, *iter));
					}
			delete nss;
		}

		//Replace the metadata ids in the Rtree payload with Rtree pageIds
		for (vector<SpatialIndex::id_type>::iterator iter = keys.begin(); iter != keys.end(); ++iter)
		{
			SpatialIndex::id_type id = *iter;

			nodeSkeleton * nss = readNode(id, rtreeStorageManager);

			if (nss != NULL)
				if (nss->nodeType == SpatialIndex::RTree::PersistentLeaf)
				{
					for(unsigned i=0; i<nss->children; i++)
					{
						MetadataEntry m = MetadataEntry(nss->m_pData[i], nss->m_pDataLength[i]);
						set<uint32_t> updatedLinks;
						for(set<uint32_t>::iterator it = m.pageLinks.begin(); it != m.pageLinks.end(); ++it)
						{
							map<uint32_t,SpatialIndex::id_type>::iterator res = mapping.find(*it);
							if(res != mapping.end())
								updatedLinks.insert(res->second);
							else
								cout << "Unable to find mapping for " << *it << endl;
						}
						m.pageLinks.swap(updatedLinks);
						int length = nss->m_pDataLength[i];
						delete[] nss->m_pData[i];
						m.serialize(&nss->m_pData[i], length);
						nss->m_pDataLength[i]=(uint32_t)length;
					}
					writeNode(id, rtreeStorageManager, nss);
				}
			delete nss;
		}

		delete rtreeStorageManager;
	}

	void SeedBuilder::writeNode(SpatialIndex::id_type page,
			SpatialIndex::IStorageManager* m_pStorageManager,
			nodeSkeleton * n)
	{

		byte* buffer = new byte[n->dataLength];;

		byte* ptr;

		ptr = buffer;

		memcpy(ptr, &n->nodeType, sizeof(uint32_t));
		ptr += sizeof(uint32_t);

		memcpy(ptr, &n->level, sizeof(uint32_t));
		ptr += sizeof(uint32_t);

		memcpy(ptr, &n->children, sizeof(uint32_t));
		ptr += sizeof(uint32_t);

		for (uint32_t u32Child = 0; u32Child < n->children; ++u32Child)
		{
			memcpy(ptr, n->m_ptrMBR[u32Child]->m_pLow, DIMENSION * sizeof(double));
			ptr += DIMENSION * sizeof(double);

			memcpy(ptr, n->m_ptrMBR[u32Child]->m_pHigh, DIMENSION * sizeof(double));
			ptr += DIMENSION * sizeof(double);

			memcpy(ptr, &(n->m_pIdentifier[u32Child]), sizeof(SpatialIndex::id_type));
			ptr += sizeof(SpatialIndex::id_type);

			memcpy(ptr, &(n->m_pDataLength[u32Child]), sizeof(uint32_t));
			ptr += sizeof(uint32_t);
			if (n->m_pDataLength[u32Child] > 0)
			{
				memcpy(ptr, n->m_pData[u32Child], n->m_pDataLength[u32Child]);
				ptr += n->m_pDataLength[u32Child];
			}
		}

		// store the node MBR for efficiency. This increases the node size a little bit.
		memcpy(ptr, n->m_nodeMBR.m_pLow, DIMENSION * sizeof(double));
		ptr += DIMENSION * sizeof(double);
		memcpy(ptr, n->m_nodeMBR.m_pHigh, DIMENSION * sizeof(double));
		ptr += DIMENSION * sizeof(double);

		//finally store that thing
		try
		{
			m_pStorageManager->storeByteArray(page, n->dataLength, buffer);
			delete[] buffer;
		}
		catch (SpatialIndex::InvalidPageException& e)
		{
			delete[] buffer;
			std::cerr << e.what() << std::endl;
			throw;
		}
	}

	nodeSkeleton * SeedBuilder::readNode(SpatialIndex::id_type page,
			SpatialIndex::IStorageManager* m_pStorageManager)
	{
		byte* buffer;
		byte* ptr;

		ptr = buffer;
		nodeSkeleton * ns = new nodeSkeleton();

		try
		{
			m_pStorageManager->loadByteArray(page, ns->dataLength, &buffer);
		}
		catch (SpatialIndex::InvalidPageException& e)
		{
			std::cerr << e.what() << std::endl;
			throw;
		}

		int c = 0;

		try
		{
			memcpy(&ns->nodeType, buffer, sizeof(uint32_t));
			if (ns->nodeType != SpatialIndex::RTree::PersistentLeaf && ns->nodeType != SpatialIndex::RTree::PersistentIndex)
			{
				delete[] buffer;
				delete ns;
				return NULL;
			}

			// skip the node type information, it is not needed.
			c += sizeof(uint32_t);

			memcpy(&ns->level, buffer + c, sizeof(uint32_t));
			c += sizeof(uint32_t);

			memcpy(&ns->children, buffer + c, sizeof(uint32_t));
			c += sizeof(uint32_t);

			// Why +1???
			int count = ns->children;
			ns->m_ptrMBR = new SpatialIndex::Region*[count];
			for (int i=0;i<count;i++) ns->m_ptrMBR[i]=NULL;
			ns->m_pIdentifier = new SpatialIndex::id_type[count];
			ns->m_pDataLength = new uint32_t[count];
			ns->m_pData = new byte*[count];
			for (int i=0;i<count;i++) ns->m_pData[i]=NULL;

			for (uint32_t u32Child = 0; u32Child < ns->children; ++u32Child)
			{
				//cout << u32Child << "> ";
				ns->m_ptrMBR[u32Child] = new SpatialIndex::Region();
				ns->m_ptrMBR[u32Child]->m_pLow = new double[3];
				ns->m_ptrMBR[u32Child]->m_pHigh = new double[3];
				memcpy(ns->m_ptrMBR[u32Child]->m_pLow, buffer + c, DIMENSION * sizeof(double));
				c += DIMENSION * sizeof(double);
				memcpy(ns->m_ptrMBR[u32Child]->m_pHigh, buffer + c, DIMENSION * sizeof(double));
				c += DIMENSION * sizeof(double);
				memcpy(&(ns->m_pIdentifier[u32Child]), buffer + c, sizeof(SpatialIndex::id_type));
				c += sizeof(SpatialIndex::id_type);

				memcpy(&(ns->m_pDataLength[u32Child]), buffer + c, sizeof(uint32_t));
				c += sizeof(uint32_t);

				if (ns->m_pDataLength[u32Child] > 0)
				{
					ns->m_pData[u32Child] = new byte[ns->m_pDataLength[u32Child]];
					memcpy(ns->m_pData[u32Child], buffer + c, ns->m_pDataLength[u32Child]);
					c += ns->m_pDataLength[u32Child];
				}
				else
				{
					ns->m_pData[u32Child] = NULL;
				}
			}

			ns->m_nodeMBR.m_pLow = new double[3];
			ns->m_nodeMBR.m_pHigh = new double[3];
			memcpy(ns->m_nodeMBR.m_pLow, buffer + c, DIMENSION * sizeof(double));
			c += DIMENSION * sizeof(double);
			memcpy(ns->m_nodeMBR.m_pHigh, buffer + c, DIMENSION * sizeof(double));

			delete[] buffer;
		}
		catch (...)
		{
			delete[] buffer;
			throw;
		}
		return ns;
	}
}
