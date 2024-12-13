#include <bits/stdc++.h>
#include "Graph_dis_mem.h"
#include "worker.h"
// #include "LearningAutomaton.h"
// #include "Agent.h"
#include "mpi.h"
#include "CuSP.h"

using namespace std;

int THETA = 100;
// double DATA_UNIT = 0.8;
double MOVE_DATA_UNIT = 10;
double train_rate = 1.0;
double default_sampling_rate = 1.;
double Budget_rate = 0.4;
bool auto_sampling_rate = false;
bool auto_local_iteration_num = true;
double overhead_limit;
int block_nums = 0;
int cut_num = 1;
int vertex_num_in_agent = 1;
int client_num = 5;
int global_iteration = 10;
int local_iteration = 1;
int switch_iteration = 5;
bool SERVER_LESS = false;
int distributed_agent_mode = 0;
int partition_mode = 0;

double DATA_UNIT = 0.000008;
string computation_algorithm = "pagerank";
bool directed_graph = true;
bool dynamic_mode = false;
double dynamic_rate = 0.;
double k_degree = -1;

int myid, numprocs;

int main(int argc, char *argv[])
{

    int namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Get_processor_name(processor_name, &namelen);


    // fprintf(stdout, "Process %d of %d is on %s\n", myid, numprocs, processor_name);
    printf("Process %d of %d is on %s\n", myid, numprocs, processor_name);
    // fflush(stdout);

    srand(time(NULL));
    // string graph_file_name = "/home/local_graph/web-Google.txt";
    // string graph_file_name = "/home/local_graph/bin/web-google.bin";
    string graph_file_name = "/data/bin/web-google.bin";
    // string graph_file_name = "/home/local_graph/livejournal";
    string network_file_name = "network/Amazon5.txt";

    int o;
    const char *optstring = "g:n:r:t:c:l:o:w:p:s:z:d:k:";
    /*
    MPI_Barrier(MPI_COMM_WORLD);
    auto be = chrono::steady_clock::now();
    MPI_Allreduce(MPI_IN_PLACE, &myid, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    printf("123");
    auto en = chrono::steady_clock::now();
    printf("worker : %f\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(en - be).count() / 1000);
    return 0;
        */
    global_iteration = numprocs * 5;

    while ((o = getopt(argc, argv, optstring)) != -1)
    {
        switch (o)
        {
        case 'g': // 训练的图文件名称
            graph_file_name = optarg;
            if (myid == 0)
                printf("[graph_file_name]: %s\n", graph_file_name.c_str());
            break;
        case 'n': // 训练的网络文件名称
            network_file_name = optarg;
            if (myid == 0)
                printf("[network_file_name]: %s\n", network_file_name.c_str());
            break;
        case 't': // 迭代次数
            global_iteration = atoi(optarg);
            if (myid == 0)
                printf("[global_iteration]: %d\n", global_iteration);
            break;
        case 'l': // 迭代次数
            local_iteration = atoi(optarg);
            auto_local_iteration_num = false;
            if (myid == 0)
                printf("[local_iteration]: %d\n", local_iteration);
            break;
        case 'c': // 迭代次数
            client_num = atoi(optarg);
            if (myid == 0)
                printf("[client_num]: %d\n", client_num);
            break;
        case 'k':
            k_degree = atof(optarg);
            if (myid == 0)
                printf("[k degree]: %f\n", k_degree);
            break;
        case 'r': // 迭代次数
        {
            double tmp_budget_rate = atof(optarg);
            if (tmp_budget_rate < 0 || tmp_budget_rate > 1)
            {
                printf("[ERROR]: %f(Budget rate must in [0, 1])\n", tmp_budget_rate);
                exit(-1);
            }
            Budget_rate = tmp_budget_rate;
            if (myid == 0)
                printf("[Budget rate]: %f\n", Budget_rate);
            break;
        }
        case 'o':
            overhead_limit = atof(optarg);
            auto_sampling_rate = true;
            if (myid == 0)
                printf("[Overhead limited]: %f\n", overhead_limit);
            if (myid == 0)
                printf("Using Auto Sampling Rate!\n");
            default_sampling_rate = 1. / global_iteration / local_iteration;
            // default_sampling_rate = 0.01;

            break;
        case 'w':
            distributed_agent_mode = atoi(optarg);
            if (myid == 0)
                printf("[distributed agent mode]: %s\n", distributed_agent_mode == 0 ? "HASH" : distributed_agent_mode == 1 ? "RANGE"
                                                                                                                            : "RANDOM");
            break;
        case 's':
            default_sampling_rate = atof(optarg);
            // default_sampling_rate = 0.1 / local_iteration;
            if (myid == 0)
                printf("[Default sampling rate]: %f\n", default_sampling_rate);
            break;
        case 'p':
            partition_mode = atoi(optarg);
            if (myid == 0)
                printf("[Partition mode]: %s\n", partition_mode == 0 ? "HASH" : partition_mode == 1 ? "RANGE"
                                                                            : partition_mode == 2   ? "RANDOM"
                                                                                                    : "FILE");
            break;
        case 'z':
        {
            computation_algorithm = optarg;
            if (computation_algorithm == "pagerank")
            {
                if (myid == 0)
                    printf("[computation algoritm] %s\n", computation_algorithm.c_str());
                DATA_UNIT = 0.000008;
                directed_graph = true;
            }
            else if (computation_algorithm == "sssp")

            {
                if (myid == 0)
                    printf("[computation algoritm] %s\n", computation_algorithm.c_str());
                DATA_UNIT = 0.000004;
                directed_graph = true;
            }
            else if (computation_algorithm == "subgraph")

            {
                if (myid == 0)
                    printf("[computation algoritm] %s\n", computation_algorithm.c_str());
                DATA_UNIT = 0.001;
                directed_graph = false;
            }
            else
            {
                printf("[ERROR]: not correct input %s\n", computation_algorithm.c_str());
                exit(-1);
            }
            break;
        }
        case 'd':
        {
            dynamic_mode = true;
            dynamic_rate = atof(optarg);
            if (myid == 0)
                printf("[dynamic mode] %f\n", dynamic_rate);

            break;
        }
        case '?':
            printf("Unknown option: %c\n", (char)optopt);
            exit(-1);
            break;
        }
    }

    Worker worker;

    worker.distributed_agent(graph_file_name, network_file_name, distributed_agent_mode);
    worker.creat_dis_graph(partition_mode, dynamic_mode);

    worker.reload_graph(partition_mode);
    MPI_Barrier(MPI_COMM_WORLD);
    worker.init_LA(global_iteration, computation_algorithm, local_iteration);
    worker.print_RAM_size();
    worker.print();
    if(dynamic_mode)
    {
        overhead_limit *= 2;
    }
    worker.train();

    if(dynamic_mode)
    {
        overhead_limit /= 2;
        worker.dynamic_reload();
        worker.graph.print_in_worker0();
        worker.train();
    }

    // vector<double> b({0.1, 0.2});
    // for (auto bb : b)
    // {
    //     k_degree = bb;
    //     if (myid == 0)
    //     {
    //         worker.log_file << "############################################################ " << endl;
    //         worker.log_file << "##### k degree = " << bb << " ##### " << endl;
    //         worker.log_file << "############################################################ " << endl;
    //     }
    //     worker.reload_graph(partition_mode);
    //     MPI_Barrier(MPI_COMM_WORLD);
    //     worker.init_LA(global_iteration, computation_algorithm, local_iteration);
    //     worker.print_RAM_size();
    //     worker.print();
    //     worker.train();
    // }

    /*
    if (myid == 0)
    {
        worker.log_file << "############################################################ " << endl;
        worker.log_file << "##### overhead constrained, Auto Local & Sampling rate ##### " << endl;
        worker.log_file << "############################################################ " << endl;
    }

    worker.reload_graph(partition_mode);
    MPI_Barrier(MPI_COMM_WORLD);
    worker.init_LA(global_iteration, computation_algorithm, local_iteration);
    worker.print_RAM_size();
    worker.print();
    worker.train();
    */

    /*
    if (myid == 0)
    {
        worker.log_file << "################################################################## " << endl;
        worker.log_file << "##### overhead constrained, Fixed Local & Auto Sampling rate ##### " << endl;
        worker.log_file << "################################################################## " << endl;
    }

    auto_local_iteration_num = false;
    vector<int> l({1, 2, 4});
    for (auto &ll : l)
    {
        local_iteration = ll;
        default_sampling_rate = 1. / global_iteration / local_iteration;

        if (myid == 0)
        {
            worker.log_file << "Local = " << ll << "\t sampling rate = " << default_sampling_rate << endl;
        }

        worker.reload_graph(partition_mode);
        MPI_Barrier(MPI_COMM_WORLD);
        worker.init_LA(global_iteration, computation_algorithm, local_iteration);
        worker.print_RAM_size();
        worker.print();
        worker.train();

        if (myid == 0)
        {
            worker.log_file << endl
                            << "**********************" << endl;
        }
    }

    if (myid == 0)
    {
        worker.log_file << "####################################### " << endl;
        worker.log_file << "##### Fixed Local & Sampling rate ##### " << endl;
        worker.log_file << "####################################### " << endl;
    }

    auto_sampling_rate = false;
    vector<double> sp({0.01, 0.02, 0.03, 0.04, 0.05});
    // vector<int> l({1, 2, 4});
    for (auto &ll : l)
        for (auto s : sp)
        {
            // double s = 0.1 / ll;
            local_iteration = ll;
            // default_sampling_rate = 1. / global_iteration / local_iteration;
            default_sampling_rate = s;
            if (myid == 0)
            {
                worker.log_file << "Local = " << ll << "\t sampling rate = " << default_sampling_rate << endl;
            }

            worker.reload_graph(partition_mode);
            MPI_Barrier(MPI_COMM_WORLD);

            worker.init_LA(global_iteration, computation_algorithm, local_iteration);

            worker.print_RAM_size();

            worker.print();
            worker.train();
            // printf("\n ***************************************************\n");
            // printf("\n ***************************************************\n");
            // printf("\n ***************************************************\n");
            // printf("\n ***************************************************\n");
            // printf("\n ***************************************************\n");
            if (myid == 0)
            {
                worker.log_file << endl
                                << "**********************" << endl;
            }
        }
*/
    MPI_Finalize();
    return 0;
}