#include <BBP/Model/Microcircuit/Microcircuit.h>
#include <BBP/Model/Experiment/Experiment.h>
#include <BBP/Model/Microcircuit/Neuron.h>
#include <BBP/Common/Math/Geometry/Rotation.h>

#include <vector>
#include "Ntree.cpp"
#include "Box.cpp"
#include "QueryGenerator.cpp"

using namespace std;

int main(int argc, const char* argv[])
{
    // Loading NeuroCurcuit
	const bbp::URI blue_config_filename(argv[1]);
	bbp::Experiment experiment;
    experiment.open(blue_config_filename);
    bbp::Microcircuit & microcircuit = experiment.microcircuit();
    const bbp::Targets & targets = experiment.targets();
    const bbp::Cell_Target target = targets.cell_target("Column");
    microcircuit.load(target, bbp::NEURONS | bbp::MORPHOLOGIES);

    //Ntree Declaration
    Box world = Box(Vertex(-1698.38,-1065.03,-1724.75),
				    Vertex( 2248.56, 1894.28, 2276.71));
    Ntree* myTree = new Ntree(world);
    QueryGenerator* queryGenerator = new QueryGenerator(world);
    string queryFile = std::string(argv[2])+".txt";
    queryGenerator->Load(queryFile);

    //Insert into Ntree
    int segmentCount=1;
    bbp::Neurons & myNeurons = microcircuit.neurons();
    bbp::Neurons::iterator myNeuronsEnd = myNeurons.end();
    for (bbp::Neurons::iterator i = myNeurons.begin(); i != myNeuronsEnd; ++i)
        {
    	bbp::Transform_3D<bbp::Micron> trafo = i->global_transform();
    	bbp::Sections mySections = i->morphology().all_sections();
    	bbp::Sections::iterator mySectionsEnd = mySections.end();
       	for (bbp::Sections::iterator s = mySections.begin(); s != mySectionsEnd; ++s)
        	{
       		bbp::Segments segments = s->segments();
       		bbp::Segments::const_iterator segments_end = segments.end();
       		for (bbp::Segments::const_iterator j = segments.begin(); j != segments_end ; ++j)
				{
       			//if (segmentCount>=30) break;
       			bbp::Vector_3D<bbp::Micron> gbegin = trafo * j->begin().center();
        		bbp::Vector_3D<bbp::Micron> gend   = trafo * j->end().center();

        		Vertex begin = Vertex(gbegin.x(),gbegin.y(),gbegin.z());
        		Vertex end   = Vertex(gend.x(),gend.y(),gend.z());
        		Cone   cone  = Cone(begin,end,j->begin().radius(),j->end().radius());
        		Vertex center;
        		Cone::center(cone,center);

        		Data   data  = Data(center,segmentCount);

        	    myTree->Insert(data);

        		if (segmentCount%500000==0) cout << "Segments Done: " << segmentCount << "\n";
           		segmentCount++;
        		}
        	}
        }
   if (HEIRISTIC==true)
	   myTree->Optimize();
   myTree->Analyze();


   cout << "\n\n\n\n\n";
   cout << "DENSITY" << "," << "NTREE SEED" << "," << "NTREE RANGE" << "\n";
   int results=0;
   for (int j=0;j<BINS;j++)
	   for (std::vector<Query>::iterator i = queryGenerator->queries[j].begin(); i != queryGenerator->queries[j].end(); ++i)
		   {
		   myTree->Seed(*i);
		   myTree->RangeSearch(*i);
		   cout << i->density << "," << i->nodeAccesses << "," << i->rangeNodeAccesses << "\n";
		   //results += i->ValidateSeeding();
		   //i->ValidateSeedingDetail();
		   }
   cout << "\n\nResults Accuracy: " << ((results+0.0)/(QUERIES+0.0)*100) << "%\n";

     // Destroy
    // myTree->~Ntree();
     return 0;
}
