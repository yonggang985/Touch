#ifndef SEED_BUILDER_HPP_
#define SEED_BUILDER_HPP_

#include <vector>
#include <SpatialIndex.h>
#include "Metadata.hpp"

namespace FLAT
{
	class nodeSkeleton
	{
	public:
		SpatialIndex::id_type *m_pIdentifier;

		SpatialIndex::Region **m_ptrMBR;

		uint32_t *m_pDataLength;

		byte **m_pData;

		SpatialIndex::Region m_nodeMBR;

		uint32_t level;

		uint32_t children;

		uint32_t nodeType;

		uint32_t dataLength;

		nodeSkeleton();

		~nodeSkeleton();
	};

	class data_stream : public SpatialIndex::IDataStream
	{
	public:
		unsigned currentMD;

		vector<MetadataEntry*> md;

		data_stream (vector<MetadataEntry*>& metadataStructure);

		virtual ~data_stream();

		virtual bool hasNext();

		virtual uint32_t size();

		virtual void rewind();

		virtual SpatialIndex::IData* getNext();
	};

	class SeedBuilder
	{
	public:
		static void buildSeedTree(std::string seedFile, SpatialIndex::IDataStream* stream);

		static void writeNode(SpatialIndex::id_type page,SpatialIndex::IStorageManager* m_pStorageManager,nodeSkeleton * n);

		static nodeSkeleton * readNode(SpatialIndex::id_type page,SpatialIndex::IStorageManager* m_pStorageManager);
	};

}
#endif
