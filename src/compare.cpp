# include <iostream>
# include <fstream>
# include <sstream>
# include "string.h"
# include "global.h"
# include <libkahypar.h>

using namespace std;

int main(int argc, char *argv[])
{
    string hgrname = argv[2];
    string outname = argv[4];


    ifstream in_file;
    ofstream out_file;
    in_file.open(hgrname);
    out_file.open(outname);


    string line;
    string num_nets;
    string num_nodes;
    int nNode;
    int nNet;
    vector<vector<int>> nets;

    int cnt = 0;


    while(getline(in_file, line))
    {
        istringstream iss(line);

        if(cnt == 0)
        {
            getline(iss, num_nets, ' ');
            getline(iss, num_nodes, ' ');
            out_file<<num_nets<<' '<<num_nodes<<endl;
            nNode = stoi(num_nodes);
            nNet = stoi(num_nets);
        }
        else
        {
            string s;
            vector<int> pins;
            while(getline(iss, s, ' '))
            {
                int node_id = stoi(s);
                pins.push_back(node_id);
                node_id = node_id + 1;
                out_file<<node_id<<' ';
            }

            nets.push_back(pins);

            out_file<<endl;
        }

        cnt++;

    }

    cout<<nNode<<endl;
    cout<<nets.size()<<endl;

    in_file.close();
    out_file.close();
    
    //PaToH

    PaToH_Parameters args;

    int *cwghts = new int[nNode];
    for(int i=0; i<nNode; i++)
        cwghts[i] = 1;

    int *nwghts = new int[nNet];
    for(int i=0; i<nNet; i++)
        nwghts[i] = 1;

    int nconst = 1;
    int useFixCells = 1;
    PaToH_Initialize_Parameters(&args, PATOH_CUTPART, PATOH_SUGPARAM_QUALITY);
    args.seed = 1;
    args._k = 4;
    args.init_imbal = 0.05;
    args.final_imbal = 0.05;

    int nPin = 0;
    for(int i=0; i<nets.size(); i++)
        nPin += nets[i].size();

    int *xpins = new int[nNet+1];
    int *pins = new int[nPin];

    for(int i=0, p=0; i < nNet; i++)
    {
        xpins[i] = p;
        for(int j=0; j<nets[i].size(); j++)
        {
            int nodeid = nets[i][j];
            pins[p++] = nodeid;
        }

    }

    xpins[nNet] = nPin;


	PaToH_Check_User_Parameters(&args, true);
    int *partvec = new int[nNode];
	int *partweights = new int[args._k*nconst];
    int cut;

    PaToH_Alloc(&args, nNode, nNet, nconst, cwghts, nwghts, xpins, pins);
    for(int i=0; i<nNode; i++)
        partvec[i] = -1;
    
	PaToH_Part(&args, nNode, nNet, nconst, useFixCells, cwghts, nwghts, xpins, pins, NULL, partvec, partweights, &cut);

    cout<<"PaToH cut size:"<<cut<<endl;

    //KaHyPar
    kahypar_context_t* context = kahypar_context_new();
    kahypar_configure_context_from_file(context, "/data/ssd/qluo/compare_partitioner/thirdparty/kahypar/config/cut_kKaHyPar_sea20.ini");

    //kahypar_set_seed(context, 42);

    const kahypar_hypernode_id_t num_vertices = nNode;
    const kahypar_hyperedge_id_t num_hyperedges = nNet;

    unique_ptr<kahypar_hyperedge_weight_t[]> hyperedge_weights = make_unique<kahypar_hyperedge_weight_t[]>(nNet);

    for(int i=0; i<nNet; i++)
        hyperedge_weights[i] = 1;

    unique_ptr<size_t[]> hyperedge_indices = make_unique<size_t[]>(nNet+1);
    unique_ptr<kahypar_hyperedge_id_t[]> hyperedges = make_unique<kahypar_hyperedge_id_t[]>(nPin);

    for(int i=0; i<nNet+1; i++)
        hyperedge_indices[i] = xpins[i];
    
    for(int i=0; i<nPin; i++)
        hyperedges[i] = pins[i];

    const double imbalance = 0.05;
    const kahypar_partition_id_t k = 4;

    kahypar_hyperedge_weight_t objective = 0;

    vector<kahypar_partition_id_t> partition(num_vertices, -1);

    kahypar_partition(num_vertices, num_hyperedges,
       	                imbalance, k,
               	        /*vertex_weights */ nullptr, hyperedge_weights.get(),
               	        hyperedge_indices.get(), hyperedges.get(),
       	                &objective, context, partition.data());
    
    cout<<"KaHyPar partition"<<endl;

    /*

    for(int i = 0; i != num_vertices; ++i) {
        cout << i << ":" << partition[i] << endl;
    }
    */

    cout<<"cut:"<<objective<<endl;

    kahypar_context_free(context);

    delete[] xpins;
	delete[] pins;
    delete[] cwghts;
	delete[] nwghts;
	delete[] partweights;
	delete[] partvec;

    PaToH_Free();


    return 0;
    
}