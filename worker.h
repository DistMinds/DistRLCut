#include "Graph_dis_mem.h"
#include <bits/stdc++.h>
#include <omp.h>
#include <mpi.h>

#ifndef __WORKER__
#define __WORKER__

extern double train_rate;
extern double Budget_rate;
extern bool auto_sampling_rate;
extern bool auto_local_iteration_num;
extern double default_sampling_rate;
extern double overhead_limit;
extern int block_nums;
extern int cut_num;
extern int vertex_num_in_agent;
extern int client_num;
extern double overhead_limit;
extern int switch_iteration;
extern double k_degree;

extern int myid;
extern int numprocs;

extern int local_iteration;

enum MPI_TAG
{
    DISTRIBUTED_AGENT_SIZE,
    DISTRIBUTED_AGENT

};

string get_time()
{
    time_t now = time(0);
    tm *ltm = localtime(&now);
    return (to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min) + ":" + to_string(ltm->tm_sec));
}
class Partitioner
{
public:
    uint32_t vertex_num = 0; // 顶点数量
    uint64_t edge_num = 0;   // 边数量

    string GRAPH_FILE_NAME;   // 图文件名
    string NETWORK_FILE_NAME; // 服务器文件名
    uint64_t file_lines;      // 图文件长度

    struct bin_file_edge
    {
        uint32_t u, v;
    };

    Partitioner(string graph_file_name, string network_file_name)
    {
        GRAPH_FILE_NAME = graph_file_name;
        NETWORK_FILE_NAME = network_file_name;
    }
    void get_graph_file_info()
    {
        ifstream file;
        file.open(GRAPH_FILE_NAME.c_str(), ios::in | ios::binary);

        // file.open;
        if (!file.is_open())
        {
            cout << "Can't not open graph file : " << GRAPH_FILE_NAME << endl;
            exit(-1);
        }

        bin_file_edge header;
        file.read((char *)(&header), sizeof(header));
        if (header.u == 1145141919)
        {

            cout << "[Partitioner] Binary files detected ^_^ " << endl;
            vertex_num = header.v;
            cout << "[Partitioner] vertex num : " << vertex_num << endl;
            file.read((char *)(&edge_num), sizeof(edge_num));
            cout << "[Partitioner] edge num : " << edge_num << endl;
            file.close();
        }
        else
            exit(0);
        // TODO: generated
    }
    vector<vector<id_type>> distribute_agent(int worker_num, int mode, unsigned int seed = 0)
    {
        enum distributed_way
        {
            HASH,
            RANGE,
            RANDOM
        };
        vector<vector<id_type>> res(worker_num);
        // Hash
        switch (mode)
        {
        case HASH:
        {

            cout << "[Graph State] using HASH mode to distribute agent : " << endl;

#pragma omp parallel num_threads(worker_num)
            {
                int k = omp_get_thread_num();
                // cout << k << endl;
                vector<id_type> &r = res[k];
                for (id_type i = 0; i < vertex_num; i++)
                {
                    if (i % worker_num == k)
                        r.push_back(i);
                }
            }

            break;
        }
        case RANGE:
        {

            cout << "[Partitioner] using RANGE mode to distribute agent : " << endl;
            
#pragma omp parallel num_threads(worker_num)
            {
                int k = omp_get_thread_num();
                vector<id_type> &r = res[k];
                id_type agent_per_worker = vertex_num / worker_num;
                for (id_type i = 0; i < vertex_num; i++)
                {
                    int dc = min(i / agent_per_worker, (id_type)worker_num - 1);
                    if (dc == k)
                        r.push_back(i);
                }
            }
            break;
            int edge_vertex_rate = edge_num / vertex_num;
            uint64_t workload_per_client = 2 * edge_num / worker_num;
            ifstream degree_file;
            string degree_file_name = GRAPH_FILE_NAME.substr(0, GRAPH_FILE_NAME.rfind('.')) + ".degree";
            degree_file.open(degree_file_name, ios::in | ios::binary);
            if (!degree_file.is_open())
            {
                cout << "Can't open degree file : " << degree_file_name << endl;
                exit(0);
            }
            uint64_t sum = 0;
            id_type last = 0;
            int index = 0;
            bin_file_edge header, tmp;
            degree_file.read((char *)(&header), sizeof(header));
            if (header.u == 1145141919)
            {
                uint64_t edge_num;

                degree_file.read((char *)(&edge_num), sizeof(edge_num));
                cout << "[degree file] edge num : " << edge_num << endl;
                // cout << "avg in degree per client : " << avg_edges_num_per_client << endl;

                for (uint64_t i = 0; i < vertex_num; i++)
                {
                    degree_file.read((char *)(&tmp), sizeof(tmp));
                    id_type in = tmp.u, out = tmp.v;
                    sum += in + edge_vertex_rate;
                    if (sum >= workload_per_client)
                    {
                        cout << sum << endl;
                        res[index].resize(i - last + 1);
                        iota(res[index].begin(), res[index].end(), last);
                        last = i + 1;

                        sum = 0;
                        index++;
                    }
                }
                if (index != worker_num)
                {
                    cout << sum << endl;
                    res[index].resize(vertex_num - last);
                    iota(res[index].begin(), res[index].end(), last);
                }
            }
            else
            {
                cout << "Not stander degree file : " << degree_file_name << endl;
                exit(0);
            }

            break;
        }
        case RANDOM:
        {

            cout << "[Graph State] using RANDOM mode to distribute agent : " << endl;
            srand(seed);
            vector<unsigned int> thread_seed(omp_get_max_threads());
            for (auto &x : thread_seed)
                x = rand();
            // cout << thread_seed.size() << endl;

            vector<int> dc_per_agent(vertex_num);
#pragma omp parallel for
            for (id_type i = 0; i < vertex_num; i++)
            {
                int k = omp_get_thread_num();
                dc_per_agent[i] = rand_r(&thread_seed[k]) % worker_num;
            }
#pragma omp parallel num_threads(worker_num)
            {
                int k = omp_get_thread_num();
                vector<id_type> &r = res[k];
                id_type agent_per_worker = vertex_num / worker_num;
                for (id_type i = 0; i < vertex_num; i++)
                {
                    if (dc_per_agent[i] == k)
                        r.push_back(i);
                }
            }

            break;
        }

        default:
            exit(0);
        }

        cout << "[Graph State] Finished dis-mem agents partition : " << endl;
        for (int i = 0; i < worker_num; i++)
        {
            printf("[worker %d] : %lu ---> ", i, res[i].size());
            for (int j = 0; j < min((int)res[i].size(), 10); j++)
                cout << res[i][j] << ' ';
            cout << " ......" << endl;
        }

        return res;
    }
};
class Worker
{
    string GRAPH_FILE_NAME;   // 图文件名
    string NETWORK_FILE_NAME; // 服务器文件名

public:
    vector<id_type> agent_in_worker;
    unsigned int agent_in_worker_num;
    Graph graph;

    vector<vector<double>> probability;
    int global_iteration;
    int local_iteration;
    ofstream log_file;
    vector<int> pre_choice;
    double alpha = 0.4; // Alpha参数

    double Budget;
    double origin_time; // 初始通信时间
    double origin_cost; // 初始开销
    double global_movecost = 0;
    double local_budget;
    double sampling_rate = 1.;

    unordered_set<id_type> local_change_set;
    uint iter_train_agent_num;
    uint global_iter_train_agent_num;
    vector<id_type> agent_order;

    Worker()
    {
        if (myid == 0)
        {
            log_file.open("LOG.txt", ios::out | ios::trunc);
            if (!log_file.is_open())
            {
                cout << "can't open LOG file" << endl;
                exit(-1);
            }
        }
    }

    void init_LA(int global_it, string computation_algorithm, int local_it = 1)
    {
        printf("worker %d begins init LA\n", myid);
        global_iteration = global_it;
        local_iteration = local_it;
        sampling_rate = default_sampling_rate;

        agent_order.resize(agent_in_worker_num);
        iota(begin(agent_order), end(agent_order), 0);
        if (k_degree != -1)
        {
            sort(agent_order.begin(), agent_order.end(), [&](id_type a, id_type b)
                 { return graph.vertex[a].in_degree!= 0 || graph.vertex[a].in_degree < graph.vertex[b].in_degree; });
            agent_order.resize((uint)(k_degree * agent_order.size()));
            // log_file << "after k degree, agent order num : " << agent_order.size() << endl;
            printf("worker %d k degree : %d / %d\n", myid, agent_order.size(), agent_in_worker_num);
            // sampling_rate *= 1. * agent_in_worker_num / agent_order.size();
        }

        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));

        // graph.print();
        if (myid == 0) // "server"
        {
            graph.display_graph_state = true;

            log_file.setf(ios::fixed);
            log_file.precision(5);

            log_file << "Graph file name: " << graph.GRAPH_FILE_NAME << endl;
            log_file << "Network file name: " << graph.NETWORK_FILE_NAME << endl;
            log_file << "Computation algorithm: " << computation_algorithm << endl;
            log_file << "Client num: " << numprocs << endl;
            log_file << "Serverless mode" << endl;
            log_file << "global iteration num : " << global_iteration << endl;
            log_file << "local iteration num : " << local_iteration << endl;
            log_file << "budget rate : " << Budget_rate << endl;
            if (auto_sampling_rate)
                log_file << "overhead limited : " << overhead_limit << endl;

            printf("[%s] client num : %d \n", get_time().c_str(), numprocs);
            printf("[%s] global iteration num : %d\n", get_time().c_str(), global_iteration);
            printf("[%s] local iteration num : %d\n", get_time().c_str(), local_iteration);
        }
        probability = vector<vector<double>>(agent_in_worker_num, vector<double>(graph.DC_num, 1. / graph.DC_num));
        pre_choice = vector<int>(agent_in_worker_num, -1);

        origin_time = graph.transfer_time;
        origin_cost = graph.transfer_cost;
        global_movecost = 0;
        count_budget();

        printf("worker %d finished init LA\n", myid);
    }

    void count_budget()
    {

        auto global_dc_vertex_num = move(graph.get_DC_vertex_num());

        Budget = 0;
        int max_price_dc = 0;                       // 记录最贵的服务器
        double max_price = graph.DC[0].UploadPrice; // 记录最贵的服务器上传价格
        for (int i = 0; i < graph.DC_num; i++)
        {
            if (max_price < graph.DC[i].UploadPrice)
                max_price_dc = i, max_price = graph.DC[i].UploadPrice;
            Budget += global_dc_vertex_num[i] * graph.DC[i].UploadPrice;
        }
        Budget -= global_dc_vertex_num[max_price_dc] * graph.DC[max_price_dc].UploadPrice;
        Budget *= Budget_rate;
        // Budget *= 10;
        Budget *= MOVE_DATA_UNIT; // Budget为将所有顶点迁移到最贵的DC所消耗的价格的budget_rate
        origin_time = graph.transfer_time;
        origin_cost = graph.transfer_cost;

        local_budget = Budget * agent_in_worker_num / graph.vertex_num;

        if (myid == 0)
        {
            printf("[%s] Budget = %f, local budget = %f\n", get_time().c_str(), Budget, local_budget);
            log_file << "Budget : " << Budget << endl;
        }
    }

    void distributed_agent(string graph_file_name, string network_file_name, int mode = 2)
    {
        GRAPH_FILE_NAME = graph_file_name;
        NETWORK_FILE_NAME = network_file_name;

        if (myid == 0)
        {
            printf("[%s] Worker 0 --> Distributing agents......\n", get_time().c_str());
            printf("[%s] numprocs = %d\n", get_time().c_str(), numprocs);
            Partitioner p(graph_file_name, network_file_name);
            p.get_graph_file_info();
            // Hash
            vector<vector<id_type>> partition_res = move(p.distribute_agent(numprocs, mode));
            if (myid == 0)
            {
                if (mode == 0)
                    log_file << "Partition agent to Worker using HASH mode" << endl;
                if (mode == 1)
                    log_file << "Partition agent to Worker using RANGE mode" << endl;
                if (mode == 2)
                    log_file << "Partition agent to Worker using RANDOM mode" << endl;
            }

            for (int c = 1; c < numprocs; c++)
            {
                id_type num = partition_res[c].size();
                MPI_Send(&num, 1, MPI_UINT32_T, c, DISTRIBUTED_AGENT_SIZE, MPI_COMM_WORLD);
                MPI_Send(partition_res[c].data(), (int)num, MPI_UINT32_T, c, DISTRIBUTED_AGENT, MPI_COMM_WORLD);
                // MPI_Send(&c, 1, MPI_INT, c, SERVER_DISTRIBUTE_AGENT2CLIENTS, MPI_COMM_WORLD);
            }
            printf("[%s] Worker 0 --> Sending agents finished.\n", get_time().c_str());

            agent_in_worker = partition_res[0];
            agent_in_worker_num = partition_res[0].size();
        }
        else
        {
            MPI_Recv(&agent_in_worker_num, 1, MPI_UINT32_T, 0, DISTRIBUTED_AGENT_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("[%s] Worker %d --> Received %d agents num.\n", get_time().c_str(), myid, agent_in_worker_num);
            agent_in_worker.resize(agent_in_worker_num);
            MPI_Recv(agent_in_worker.data(), agent_in_worker_num, MPI_UINT32_T, 0, DISTRIBUTED_AGENT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("[%s] Worker %d --> Received %ld agents finished.\n", get_time().c_str(), myid, agent_in_worker.size());
        }
    }
    double kBToGB(long kB)
    {
        const double MB = 1024.0;
        const double GB = MB * 1024.0;

        return kB / GB;
    }
    void write_RAM_size(ofstream &s)
    {
        std::ifstream statusFile("/proc/self/status");
        std::string line;

        double virtualMemorySizeGB;
        double residentSetSizeGB;

        while (std::getline(statusFile, line))
        {
            if (line.find("VmSize:") != std::string::npos)
            {
                long virtualMemorySizeKB = std::stol(line.substr(8));
                virtualMemorySizeGB = kBToGB(virtualMemorySizeKB);
                // std::cout << "\tVirtual memory size: " << virtualMemorySizeGB << " GB" << std::endl;
            }
            else if (line.find("VmRSS:") != std::string::npos)
            {
                long residentSetSizeKB = std::stol(line.substr(7));
                residentSetSizeGB = kBToGB(residentSetSizeKB);
                // std::cout << "\tResident set size: " << residentSetSizeGB << " GB" << std::endl;
            }
        }
        std::vector<double> allVirtualMemorySizes(numprocs);
        std::vector<double> allResidentSetSizes(numprocs);

        MPI_Gather(&virtualMemorySizeGB, 1, MPI_DOUBLE, allVirtualMemorySizes.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&residentSetSizeGB, 1, MPI_DOUBLE, allResidentSetSizes.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        if (myid == 0)
        {
            for (int i = 0; i < numprocs; i++)
            {
                s << "Worker " << i << " - Virtual memory size: " << allVirtualMemorySizes[i] << " GB, "
                  << "Resident set size: " << allResidentSetSizes[i] << " GB" << std::endl;
            }
        }
    }
    void print_RAM_size()
    {
        std::ifstream statusFile("/proc/self/status");
        std::string line;

        for (int i = 0; i < numprocs; i++)
        {
            if (myid == i)
            {
                std::cout << "Worker " << myid << std::endl;
                while (std::getline(statusFile, line))
                {
                    if (line.find("VmSize:") != std::string::npos)
                    {
                        long virtualMemorySizeKB = std::stol(line.substr(8));
                        double virtualMemorySizeGB = kBToGB(virtualMemorySizeKB);
                        std::cout << "\tVirtual memory size: " << virtualMemorySizeGB << " GB" << std::endl;
                    }
                    else if (line.find("VmRSS:") != std::string::npos)
                    {
                        long residentSetSizeKB = std::stol(line.substr(7));
                        double residentSetSizeGB = kBToGB(residentSetSizeKB);
                        std::cout << "\tResident set size: " << residentSetSizeGB << " GB" << std::endl;
                    }
                }
            }
            // 等待其他 rank 完成
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    void reload_graph(int part_DC_mode = 0)
    {
        for (auto &dc : graph.DC)
            dc.reset();
        graph.reset_Mirror();
        graph.paritition_to_DC(part_DC_mode);
        graph.movecost = 0;
        if (myid == 0)
        {
            if (part_DC_mode == 0)
                log_file << "Partition vertex to DC using HASH mode" << endl;
            if (part_DC_mode == 1)
                log_file << "Partition vertex to DC using RANGE mode" << endl;
            if (part_DC_mode == 2)
                log_file << "Partition vertex to DC using RANDOM mode" << endl;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        // cout << 111;

        graph.comm_update_init_DC();
        // cout << 222;
        // graph.comm_update_high_degree_node();
        // cout << 333;
        // graph.resize_out_edge();
        graph.hybrid_cut();

        // return;

        graph.calculate_wan();

        graph.calculate_time_cost();
    }
    void dynamic_reload()
    {
        graph.dynamic_reloads();
        for (auto &dc : graph.DC)
            dc.reset();
        graph.reset_Mirror();
        graph.hybrid_cut();

        graph.movecost = 0;
        
        sampling_rate = default_sampling_rate;

        // return;

        graph.calculate_wan();

        graph.calculate_time_cost();
        origin_time = graph.transfer_time;
        origin_cost = graph.transfer_cost;
        if (myid == 0)
            log_file << "Dynamic Finished\n";
        printf("worker %d change agent order : %d --> %d\n", myid, agent_order.size(), graph.dynamic_vertex.size());
        agent_order = graph.dynamic_vertex;
    }
    void creat_dis_graph(int part_DC_mode = 0, bool dynamic = false)
    {

        graph = Graph(GRAPH_FILE_NAME, NETWORK_FILE_NAME, myid, numprocs, myid == 0 ? true : false);
        graph.read_file(agent_in_worker, dynamic);
        MPI_Barrier(MPI_COMM_WORLD);
        graph.get_comm_dest();

        MPI_Barrier(MPI_COMM_WORLD);
        // cout << 000;

        graph.paritition_to_DC(part_DC_mode);
        if (myid == 0)
        {
            if (part_DC_mode == 0)
                log_file << "Partition vertex to DC using HASH mode" << endl;
            if (part_DC_mode == 1)
                log_file << "Partition vertex to DC using RANGE mode" << endl;
            if (part_DC_mode == 2)
                log_file << "Partition vertex to DC using RANDOM mode" << endl;
            if (part_DC_mode == 3)
                log_file << "Partition vertex to DC using FILE" << endl;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        // cout << 111;

        graph.comm_update_init_DC();
        // cout << 222;
        graph.comm_update_high_degree_node();
        // cout << 333;
        graph.resize_out_edge();
        // graph.hybrid_cut();

        // // return;

        // graph.calculate_wan();

        // graph.calculate_time_cost();

        // graph.print();
        // graph.print_in_order();
        // graph.print_top_1K_mirror_info();
        // graph.local_move_test();
        // cout << 111;;
        // graph.global_move_test();

        // graph.print_top_1K_mirror_info();
    }
    void print()
    {
        graph.print_in_worker0();
        if (myid == 0)
        {
            printf("totalcost / budget : %f / %f\n", global_movecost, Budget);
        }
    }
    bool check_shutdown(double init_DTT, double current_DTT)
    {
        return init_DTT * 0.8 >= current_DTT;
    }
    bool check_shutdown(vector<double> &his, int cur_it)
    {
        return false;
        int check_num = global_iteration / 10;
        if (cur_it < check_num)
            return false;

        double sum = 0;
        for (int i = cur_it - check_num + 1; i <= cur_it; i++)
            sum += his[i];
        double mean = sum / check_num;

        double variance = 0.0;
        for (int i = cur_it - check_num + 1; i <= cur_it; i++)
        {
            variance = variance + pow(his[i] - mean, 2);
        }
        variance = variance / check_num;
        double standard_deviation = sqrt(variance);

        if (myid == 0)
        {
            for (auto &x : his)
                cout << x << ' ';
            cout << endl;
            cout << check_num << ' ' << sum << ' ' << variance << ' ' << standard_deviation << endl;
        }

        if (standard_deviation > 0.0025)
            return false;
        return true;
    }
    void train()
    {

        omp_set_num_threads(omp_get_max_threads() / 4); 
        printf("[%s] Worker %d begins training.\n", get_time().c_str(), myid);

        double client_train_time = 0;
        double client_update_time = 0;

        write_RAM_size(log_file);

        if (myid == 0)
        {
            log_file << "global it\ttime\tcost\tmovecost\tsp_rate\tchange_num\tcomputation_t\tcomm_t\tit_overhead" << endl;
            log_file << "init" << '\t' << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << graph.movecost << endl;
        }

        vector<double> sampling_rate_per_iteration(global_iteration);
        // vector<double> overhead_per_iteration(global_iteration);
        vector<double> samplingrate_overhead_rate(global_iteration);
        vector<double> DTT_normalized_his(global_iteration);
        double left_overhead = overhead_limit;
        double overhead_use = 0;
        // int last_change_num = graph.vertex_num;
        double last_change_rate = 1.;
        double init_DTT = graph.transfer_time;

        MPI_Barrier(MPI_COMM_WORLD);
        auto training_begin = chrono::steady_clock::now();

        vector<double> local_overhead_record;
        vector<double> local_samplingrate_record;
        vector<int> local_L_record;

        double used_time;

        for (int global_it = 0; global_it < global_iteration; global_it++)
        {
            double last_DTT = graph.transfer_time;

            // MPI_Barrier(MPI_COMM_WORLD);

            local_samplingrate_record.push_back(sampling_rate);
            local_L_record.push_back(local_iteration);

            auto client_train_begin = chrono::steady_clock::now();
            auto iteration_begin = chrono::steady_clock::now();
            used_time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - training_begin).count() / 1000;
            // local_overhead_record.push_back(used_time);
            // train_agents_batching(global_it);
            // train_all_agents(global_it);
            // printf("worker %d begins sampling training\n", myid);
            train_random_agents_sampling(global_it);

            auto local_train_overhead = (double)std::chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - client_train_begin).count() / 1000;
            used_time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - training_begin).count() / 1000;
            // local_overhead_record.push_back(used_time);
            local_overhead_record.push_back(local_train_overhead);

            // printf("worker %d ends sampling training\n", myid);
            // train_random_agents_sampling_with_Kparts(global_it);
            // for (auto &x : graph.DC)
            // {
            //     printf("gu %.32f gd %.32f au %.32f ad %.32f \n", x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
            // }
            ;
            MPI_Barrier(MPI_COMM_WORLD);
            used_time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - training_begin).count() / 1000;
            // local_overhead_record.push_back(used_time);

            // printf("worker %d begins global env update\n", myid);

            auto client_train_end = chrono::steady_clock::now();
            auto iter_client_train_time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_train_end - client_train_begin).count() / 1000;
            client_train_time += iter_client_train_time;

            auto client_update_begin = chrono::steady_clock::now();

            int change_size = local_change_set.size(), global_change_size;
            MPI_Allreduce(&change_size, &global_change_size, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

            MPI_Allreduce(&graph.movecost, &global_movecost, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
            MPI_Allreduce(&iter_train_agent_num, &global_iter_train_agent_num, 1, MPI_UINT32_T, MPI_SUM, MPI_COMM_WORLD);
            graph.update_global_env(local_change_set);

            auto local_update_overhead = (double)std::chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - client_update_begin).count() / 1000;
            used_time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - training_begin).count() / 1000;
            // local_overhead_record.push_back(used_time);
            local_overhead_record.push_back(local_update_overhead);

            MPI_Barrier(MPI_COMM_WORLD);

            // printf("worker %d ends global env update\n", myid);
            auto client_update_end = chrono::steady_clock::now();
            double iter_client_update_time = (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_update_end - client_update_begin).count() / 1000;
            client_update_time += iter_client_update_time;

            auto iteration_end = chrono::steady_clock::now();
            double iteration_use_overhead = (double)std::chrono::duration_cast<std::chrono::milliseconds>(iteration_end - iteration_begin).count() / 1000;

            // overhead_per_iteration[global_it] = iteration_use_overhead;
            overhead_use += iteration_use_overhead - iter_client_update_time;
            // samplingrate_overhead_rate[global_it] = sampling_rate / iteration_use_overhead;
            samplingrate_overhead_rate[global_it] = sampling_rate / (local_train_overhead);

            // for (auto &x : graph.DC)
            // {
            //     printf("gu %.32f gd %.32f au %.32f ad %.32f \n", x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
            // }
            double cur_change_rate = change_size * 1. / iter_train_agent_num;
            double global_sampling_rate = 1. * global_iter_train_agent_num / graph.vertex_num;
            // printf("%d %d\n", iter_train_agent_num, global_iter_train_agent_num);
            if (myid == 0)
            {
                // for (int i = 0; i < numprocs; i++)
                // {
                //     printf("[%s] Client 0 --> Receive %d local changes from client %d\n", get_time().c_str(), client_local_change_size[i], i);
                // }

                log_file << global_it + 1 << '\t' << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << global_movecost << '\t' << global_sampling_rate << '\t' << global_change_size << '\t' << iter_client_train_time << '\t' << local_update_overhead << '\t' << iteration_use_overhead << endl;

                printf("\n\n\n\n[LearningAutomaton] iteration : %d / %d\n", global_it + 1, global_iteration);
                printf("[LearningAutomaton] time : %.32f (%f)\n", graph.transfer_time, graph.transfer_time / origin_time);
                printf("[LearningAutomaton] cost : %.32f (%f)\n", graph.transfer_cost, graph.transfer_cost / origin_cost);
                printf("[LearningAutomaton] totalcost / budget : %f / %f\n", global_movecost, Budget);
                // if (auto_sampling_rate)
                printf("[LearningAutomaton] sampling rate : %f\n", global_sampling_rate);
                printf("[LearningAutomaton] global change nums : %d\n", global_change_size);
                printf("[LearningAutomaton] local change rate : %f\n", cur_change_rate);
                printf("[LearningAutomaton] iteration use : %f s\n", iteration_use_overhead);
                // graph.print(false);
            }
            auto training_end = chrono::steady_clock::now();
            // left_overhead = overhead_limit - ((double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000);
            left_overhead -= iter_client_train_time;

            // auto shutdown
            DTT_normalized_his[global_it] = graph.transfer_time / init_DTT;
            // if (check_shutdown(DTT_normalized_his, global_it))
            if (false && check_shutdown(init_DTT, graph.transfer_time))
            {
                if (myid == 0)
                {
                    printf("[LearningAutomaton] ******************************************************************\n");
                    printf("[LearningAutomaton] Convergence detected, algorithm terminated\n");
                    printf("[LearningAutomaton] ******************************************************************\n");
                }
                break;
            }

            MPI_Barrier(MPI_COMM_WORLD);
            // int cur_change_num = change_size;

            if (auto_sampling_rate && global_it != global_iteration - 1)
            {
                // printf("%f %f %f\n", local_train_overhead, iter_client_train_time, left_overhead);
                if (auto_local_iteration_num)
                    // update_sampling_rate(global_it, local_train_overhead + local_update_overhead, iteration_use_overhead, left_overhead, cur_change_rate, last_change_rate);
                    update_sampling_rate(global_it, local_train_overhead, iter_client_train_time, left_overhead, cur_change_rate, last_change_rate);
                else
                    update_sampling_rate(global_it, samplingrate_overhead_rate, left_overhead);
            }
            // update_sampling_rate(global_it, samplingrate_overhead_rate, left_overhead);
            // update_sampling_rate(global_it, local_train_overhead + local_update_overhead, iteration_use_overhead, left_overhead, last_DTT, graph.transfer_time);

            last_change_rate = cur_change_rate;
            MPI_Barrier(MPI_COMM_WORLD);

            // graph.print_in_worker0();
            // printf("Worker %d : %f / %f \n", myid, graph.movecost, local_budget);
            // printf("\n\n");
        }
        // graph.print_in_order();
        graph.print_in_worker0();
        if (myid == 0)
        {
            auto training_end = chrono::steady_clock::now();

            printf("\n\n\n\n[LeanrningAutomaton] client training use : %f s\n", client_train_time);
            printf("[LeanrningAutomaton] client update use : %f s\n", client_update_time);

            printf("[LeanrningAutomaton] total training use : %f s\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000);

            log_file << "client training use :\t" << client_train_time << endl;
            log_file << "client update use :\t" << client_update_time << endl;
            log_file << "total training use :\t" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000 << endl;
        }

        // printf("Worker %d local move cost %f\n", myid, graph.movecost);
        MPI_Barrier(MPI_COMM_WORLD);

        /*
        if (myid == 0)
        {
            printf("Global\t");
            for (int i = 0; i < numprocs; i++)
                printf("wokrer_%d\t", i);
            // printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
        for (int i = 0; i < global_iteration; i++)
        {
            if (myid == 0)
                printf("\n%d_traing_begin\t", i);
            for (int j = 0; j < numprocs; j++)
            {
                if (myid == j)
                    printf("%f\t", local_overhead_record[i * 4 + 0]);
                MPI_Barrier(MPI_COMM_WORLD);
            }
            if (myid == 0)
                printf("\n%d_traing_end\t", i);
            for (int j = 0; j < numprocs; j++)
            {
                if (myid == j)
                    printf("%f\t", local_overhead_record[i * 4 + 1]);
                MPI_Barrier(MPI_COMM_WORLD);
            }
            if (myid == 0)
                printf("\n%d_update_begin\t", i);
            for (int j = 0; j < numprocs; j++)
            {
                if (myid == j)
                    printf("%f\t", local_overhead_record[i * 4 + 2]);
                MPI_Barrier(MPI_COMM_WORLD);
            }
            if (myid == 0)
                printf("\n%d_update_end\t", i);
            for (int j = 0; j < numprocs; j++)
            {
                if (myid == j)
                    printf("%f\t", local_overhead_record[i * 4 + 3]);
                MPI_Barrier(MPI_COMM_WORLD);
            }
            // if (myid == 0)
            //     printf("\n");
        }
        if (myid == 0)
            printf("\n");
        */

        if (true)
        {

            if (myid == 0)
            {
                printf("\nworker");
                for (int i = 0; i < global_iteration; i++)
                {
                    printf("\ttrain\tupdate");
                }
                printf("\n");
            }
            MPI_Barrier(MPI_COMM_WORLD);
            for (int i = 0; i < numprocs; i++)
            {
                if (i == myid)
                {
                    printf("worker_%d", i);
                    for (auto &x : local_overhead_record)
                        printf("\t%f", x);
                    printf("\n");
                    sleep(1);
                }
                MPI_Barrier(MPI_COMM_WORLD);
            }

            if (myid == 0)
            {
                printf("\nworker");
                for (int i = 0; i < global_iteration; i++)
                {
                    printf("\tsampling_rate");
                }
                printf("\n");
            }
            MPI_Barrier(MPI_COMM_WORLD);
            for (int i = 0; i < numprocs; i++)
            {
                if (i == myid)
                {
                    printf("worker_%d", i);
                    for (auto &x : local_samplingrate_record)
                        printf("\t%f", x);
                    printf("\n");
                    sleep(1);
                }
                MPI_Barrier(MPI_COMM_WORLD);
            }

            if (myid == 0)
            {
                printf("\nworker");
                for (int i = 0; i < global_iteration; i++)
                {
                    printf("\tlocal_it");
                }
                printf("\n");
            }
            MPI_Barrier(MPI_COMM_WORLD);
            for (int i = 0; i < numprocs; i++)
            {
                if (i == myid)
                {
                    printf("worker_%d", i);
                    for (auto &x : local_L_record)
                        printf("\t%d", x);
                    printf("\n");
                    sleep(1);
                }
                MPI_Barrier(MPI_COMM_WORLD);
            }
        }
    }

    void graph_local_move_test()
    {
        MPI_Barrier(MPI_COMM_WORLD);
        auto time_begin = chrono::steady_clock::now();
        for (id_type vv = 0; vv < agent_order.size(); vv++)
        // for (id_type vv = 0; vv < graph.vertex_num; vv++)
        {
            id_type v = agent_order[vv];
            // if(vertex[v].i`s_high_degree)
            graph.moveVertex(v, 0);
        }
        graph.print_in_order();
        for (id_type vv = 0; vv < agent_order.size(); vv++)
        {
            id_type v = agent_order[vv];
            graph.moveVertex(v, graph.vertex[v].init_dc);
        }
        graph.print_in_order();
        auto time_end = chrono::steady_clock::now();
        cout << graph.sum << endl;

        cout << myid << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_begin).count() / 1000 << endl;
    }

    void train_all_agents(int global_it)
    {
        local_change_set.clear();
        iter_train_agent_num = 0;
        for (int local_it = 0; local_it < local_iteration; local_it++)
        {
            // auto shuffle_begin = chrono::steady_clock::now();
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));

            make_decision_for_all_agents(global_it, local_it);
            // auto shuffle_end = chrono::steady_clock::now();
            // cout << myid << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(shuffle_end - shuffle_begin).count() / 1000 << endl;

            uint sum = 0, change = 0;

            for (id_type vv = 0; vv < agent_order.size(); vv++)
            // for(id_type i = 0; i < agent_vector.size(); i++)
            {
                id_type v = agent_order[vv];
                int origin_dc = graph.vertex[v].current_dc;
                double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
                int choice_dc = pre_choice[v];
                // int choice_dc = 0;
                // if (choice_dc == -1)
                //     continue;
                // if(vv < 10)
                //     printf("%d ", graph.vertex[v].id);
                // if (graph.vertex[v].id == 12)//12
                //     for (auto &x : graph.DC)
                //     {
                //         printf("if move to DC %d gu %.32f gd %.32f au %.32f ad %.32f \n", origin_dc, x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
                //     }
                graph.moveVertex(v, choice_dc);
                // if (graph.vertex[v].id == 12)//12
                //     for (auto &x : graph.DC)
                //     {
                //         printf("if move to DC %d gu %.32f gd %.32f au %.32f ad %.32f \n", choice_dc, x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
                //     }
                iter_train_agent_num++;
                double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;
                // if (graph.vertex[v].id == 12)//12
                //     cout << old_time << ' ' << old_cost << ' ' << old_mvcost << endl
                //          << new_time << ' ' << new_cost << ' ' << new_mvcost << endl;

                if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, global_it) > 0)
                {
                    for (int dc = 0; dc < graph.DC_num; dc++)
                    {
                        if (choice_dc == dc)
                            probability[v][dc] += alpha * (1 - probability[v][dc]);
                        else
                            probability[v][dc] = (1 - alpha) * probability[v][dc];
                    }
                    local_change_set.insert(v);
                    change++;
                    // if(graph.vertex[v].id == 700)
                    //     cout << "MOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVED" << choice_dc << endl;
                }
                else
                {
                    graph.moveVertex(v, origin_dc);
                    // if(graph.vertex[v].id == 252)
                    //     cout << "BACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACK" << origin_dc<< endl;
                }
            }
            // auto shuffle_end = chrono::steady_clock::now();
            // cout << myid << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(shuffle_end - shuffle_begin).count() / 1000 << " --- " << 1. * change / sum << endl;
        }
    }
    void train_random_agents_sampling(int global_it)
    {
        local_change_set.clear();
        iter_train_agent_num = 0;
        if (sampling_rate == 0.)
            return;
        for (int local_it = 0; local_it < local_iteration; local_it++)
        {
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
            // auto shuffle_begin = chrono::steady_clock::now();
            // make_decision_for_all_agents(local_it);
            make_decision_for_sampling_agents(global_it, local_it);
            // auto shuffle_end = chrono::steady_clock::now();
            // cout << myid << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(shuffle_end - shuffle_begin).count() / 1000 << endl;

            uint sum = 0, change = 0;
            uint sampling_agents_num = sampling_rate * agent_order.size();

            for (auto vv = 0; vv < sampling_agents_num; vv++)
            // for(id_type i = 0; i < agent_vector.size(); i++)
            {
                id_type v = agent_order[vv];
                int origin_dc = graph.vertex[v].current_dc;
                double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
                int choice_dc = pre_choice[v];
                // int choice_dc = 0;
                // if (choice_dc == -1)
                //     continue;
                // if(vv < 10)
                //     printf("%d ", graph.vertex[v].id);
                // if (graph.vertex[v].id == 12) // 12
                //     for (auto &x : graph.DC)
                //     {
                //         printf("if move to DC %d gu %.32f gd %.32f au %.32f ad %.32f \n", origin_dc, x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
                //     }
                graph.moveVertex(v, choice_dc);
                // if (graph.vertex[v].id == 12) // 12
                //     for (auto &x : graph.DC)
                //     {
                //         printf("if move to DC %d gu %.32f gd %.32f au %.32f ad %.32f \n", choice_dc, x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
                //     }
                iter_train_agent_num++;
                double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;
                // if (graph.vertex[v].id == 12) // 12
                //     cout << old_time << ' ' << old_cost << ' ' << old_mvcost << endl
                //          << new_time << ' ' << new_cost << ' ' << new_mvcost << endl;

                if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, global_it) > 0)
                {
                    for (int dc = 0; dc < graph.DC_num; dc++)
                    {
                        if (choice_dc == dc)
                            probability[v][dc] += alpha * (1 - probability[v][dc]);
                        else
                            probability[v][dc] = (1 - alpha) * probability[v][dc];
                    }
                    local_change_set.insert(v);
                    change++;
                    // if(graph.vertex[v].id == 700)
                    //     cout << "MOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVED" << choice_dc << endl;
                }
                else
                {
                    graph.moveVertex(v, origin_dc);
                    // if(graph.vertex[v].id == 252)
                    //     cout << "BACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACK" << origin_dc<< endl;
                }
            }
            // auto shuffle_end = chrono::steady_clock::now();
            // cout << myid << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(shuffle_end - shuffle_begin).count() / 1000 << " --- " << 1. * change / sum << endl;
        }
    }
    void train_random_agents_sampling_with_Kparts(int global_it)
    {
        local_change_set.clear();
        for (int local_it = 0; local_it < local_iteration; local_it++)
        {
            // auto shuffle_begin = chrono::steady_clock::now();
            // make_decision_for_all_agents(local_it);
            // make_decision_for_sampling_agents(local_it);
            const int Kparts = 10;
            for (int k = 0; k < Kparts; k++)
            {
                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
                make_decision_for_sampling_agents_Kparts(global_it, local_it, k, Kparts);
                // auto shuffle_end = chrono::steady_clock::now();
                // cout << myid << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(shuffle_end - shuffle_begin).count() / 1000 << endl;

                uint sum = 0, change = 0;
                uint sampling_agents_num = sampling_rate * agent_in_worker_num / Kparts;

                for (auto vv = k * Kparts; vv < (k + 1) * sampling_agents_num; vv++)
                // for(id_type i = 0; i < agent_vector.size(); i++)
                {
                    id_type v = agent_order[vv];
                    int origin_dc = graph.vertex[v].current_dc;
                    double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
                    int choice_dc = pre_choice[v];
                    // int choice_dc = 0;
                    // if (choice_dc == -1)
                    //     continue;
                    // if(vv < 10)
                    //     printf("%d ", graph.vertex[v].id);
                    // if (graph.vertex[v].id == 12)//12
                    //     for (auto &x : graph.DC)
                    //     {
                    //         printf("if move to DC %d gu %.32f gd %.32f au %.32f ad %.32f \n", origin_dc, x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
                    //     }
                    graph.moveVertex(v, choice_dc);
                    // if (graph.vertex[v].id == 12)//12
                    //     for (auto &x : graph.DC)
                    //     {
                    //         printf("if move to DC %d gu %.32f gd %.32f au %.32f ad %.32f \n", choice_dc, x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
                    //     }
                    sum++;
                    double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;
                    // if (graph.vertex[v].id == 12)//12
                    //     cout << old_time << ' ' << old_cost << ' ' << old_mvcost << endl
                    //          << new_time << ' ' << new_cost << ' ' << new_mvcost << endl;

                    if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, global_it) > 0)
                    {
                        for (int dc = 0; dc < graph.DC_num; dc++)
                        {
                            if (choice_dc == dc)
                                probability[v][dc] += alpha * (1 - probability[v][dc]);
                            else
                                probability[v][dc] = (1 - alpha) * probability[v][dc];
                        }
                        local_change_set.insert(v);
                        change++;
                        // if(graph.vertex[v].id == 700)
                        //     cout << "MOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVEDMOVED" << choice_dc << endl;
                    }
                    else
                    {
                        graph.moveVertex(v, origin_dc);
                        // if(graph.vertex[v].id == 252)
                        //     cout << "BACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACKBACK" << origin_dc<< endl;
                    }
                }
            }
            // auto shuffle_end = chrono::steady_clock::now();
            // cout << myid << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(shuffle_end - shuffle_begin).count() / 1000 << " --- " << 1. * change / sum << endl;
        }
    }

    void make_decision_for_all_agents(int global_it, int local_it)
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
        // return;
        // printf("%d\n", agent_in_worker_num);
#pragma omp parallel for
        // for (id_type vv = 0; vv < client_thread_num; vv++)
        for (id_type vv = 0; vv < agent_order.size(); vv++)
        {
            id_type v = agent_order[vv];
            vector<double> s = move(moveVirtualVertex(v, global_it)); // local iteration 默认为1了
            int max_s = max_element(s.begin(), s.end()) - s.begin();
            for (int dc = 0; dc < graph.DC_num; dc++)
            {
                if (max_s == dc)
                    probability[v][dc] += alpha * (1 - probability[v][dc]);
                else
                    probability[v][dc] = (1 - alpha) * probability[v][dc];
            }
            pre_choice[v] = make_decision_greedy(v);
            // if (graph.vertex[v].id == 12)//12
            // {
            //     cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
            //     for (auto &ss : s)
            //         printf("%.32f ", ss);

            //     cout << endl
            //          << pre_choice[v] << endl;
            //     ;
            // }
        }
    }
    void make_decision_for_sampling_agents(int global_it, int local_it)
    {
        // return;
        // printf("%d\n", agent_in_worker_num);

        uint sampling_agents_num = sampling_rate * agent_order.size();
#pragma omp parallel for
        // for (id_type vv = 0; vv < client_thread_num; vv++)
        for (id_type vv = 0; vv < sampling_agents_num; vv++)
        {
            id_type v = agent_order[vv];
            vector<double> s = move(moveVirtualVertex(v, global_it)); // local iteration 默认为1了
            int max_s = max_element(s.begin(), s.end()) - s.begin();
            for (int dc = 0; dc < graph.DC_num; dc++)
            {
                if (max_s == dc)
                    probability[v][dc] += alpha * (1 - probability[v][dc]);
                else
                    probability[v][dc] = (1 - alpha) * probability[v][dc];
            }
            pre_choice[v] = make_decision_greedy(v);
            // if (graph.vertex[v].id == 12) // 12
            // {
            //     cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
            //     cout << graph.vertex[v].id << ' ' << (graph.vertex[v].is_high_degree ? "High Degree Node " : "Low Degree Node ") << " In degree : " << graph.vertex[v].in_degree << " Out Degree : " << graph.vertex[v].out_degree << endl;
            //     cout << "In edges : " << graph.vertex[v].in_edge.size() << " Out edges : " << graph.vertex[v].out_edge.size() << endl;
            //     for (auto &ss : s)
            //         printf("%.32f ", ss);

            //     cout << endl
            //          << pre_choice[v] << endl;
            //     ;
            // }
            // if (myid == 0)
            // {
            //     if (vv < 20)
            //     {
            //         printf("%d : ", graph.vertex[v].id);
            //         for (auto &ss : s)
            //             printf("%.32f ", ss);
            //         printf("\n");
            //     }
            // }
        }
    }
    void make_decision_for_sampling_agents_Kparts(int global_it, int local_it, int k, int Kparts)
    {
        // return;
        // printf("%d\n", agent_in_worker_num);

        uint sampling_agents_num = sampling_rate * agent_in_worker_num / Kparts;
#pragma omp parallel for
        // for (id_type vv = 0; vv < client_thread_num; vv++)
        for (id_type vv = k * sampling_agents_num; vv < (k + 1) * sampling_agents_num; vv++)
        {
            id_type v = agent_order[vv];
            vector<double> s = move(moveVirtualVertex(v, global_it)); // local iteration 默认为1了
            int max_s = max_element(s.begin(), s.end()) - s.begin();
            for (int dc = 0; dc < graph.DC_num; dc++)
            {
                if (max_s == dc)
                    probability[v][dc] += alpha * (1 - probability[v][dc]);
                else
                    probability[v][dc] = (1 - alpha) * probability[v][dc];
            }
            pre_choice[v] = make_decision_greedy(v);
            // if (graph.vertex[v].id == 12)//12
            // {
            //     cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
            //     for (auto &ss : s)
            //         printf("%.32f ", ss);

            //     cout << endl
            //          << pre_choice[v] << endl;
            //     ;
            // }
        }
    }
    inline int make_decision_greedy(id_type &agent_index)
    {
        int choice_dc = max_element(probability[agent_index].begin(), probability[agent_index].end()) - probability[agent_index].begin();
        return choice_dc;
    }

    vector<double> moveVirtualVertex(id_type v, int it)
    // 以当前环境为参考，移动顶点到所有dc，返回每个dc的score情况
    {
        int &DC_num = graph.DC_num;
        vector<Vertex> &vertex = graph.vertex;

        vector<double> score(DC_num);

        // unordered_map<id_type, int>
        // if (vertex[v].id == 12) // 12
        // {

        //     // printf("=== SCORE === : dc %d, score = %f, %f %f %f %f %f %f\n", dc, score[dc], graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost);
        //     printf("INIT DC\n");
        //     for (auto &i : graph.DC)
        //     {
        //         printf("DC : gu %f\tgd %f\tau %f\tad %f\n", i.gather_upload_wan, i.gather_download_wan, i.apply_upload_wan, i.apply_download_wan);
        //     }
        // }

        for (int dc = 0; dc < DC_num; dc++)
        {
            if (vertex[v].current_dc == dc)
                continue;
            double movecost = graph.movecost;
            vector<DataCenter> DC = graph.DC;

            int origin_dc = vertex[v].current_dc;
            int init_dc = vertex[v].init_dc;
            vector<std::vector<Mirror>> &mirror = graph.mirror;
            vector<std::vector<Mirror>> &mirror_delta = graph.mirror_delta;

            if (origin_dc == init_dc)
            {
                movecost += MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
            }
            else if (dc == init_dc)
            {
                movecost -= MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
            }
            if (vertex[v].is_high_degree)
            {
                for (int i = 0; i < DC_num; i++)
                {
                    if (i == vertex[v].current_dc)
                        continue;
                    if (mirror[i][v].in_degree > 0)
                        DC[origin_dc].gather_download_wan -= DATA_UNIT;
                    if (mirror[i][v].out_degree > 0)
                        DC[origin_dc].apply_upload_wan -= DATA_UNIT;
                }
                int mirrorin = mirror[origin_dc][v].in_degree;
                int mirrorout = mirror[origin_dc][v].out_degree;

                if (mirrorin > 0)
                    DC[origin_dc].gather_upload_wan += DATA_UNIT;
                if (mirrorout > 0)
                    DC[origin_dc].apply_download_wan += DATA_UNIT;

                if (mirror[dc][v].in_degree > 0)
                    DC[dc].gather_upload_wan -= DATA_UNIT;
                if (mirror[dc][v].out_degree > 0)
                    DC[dc].apply_download_wan -= DATA_UNIT;

                for (int i = 0; i < DC_num; i++)
                {
                    if (i != dc && i != origin_dc && mirror[i][v].in_degree > 0)
                        DC[dc].gather_download_wan += DATA_UNIT;
                    if (i != dc && i != origin_dc && mirror[i][v].out_degree > 0)
                        DC[dc].apply_upload_wan += DATA_UNIT;
                }

                if (mirrorin > 0)
                    DC[dc].gather_download_wan += DATA_UNIT;
                if (mirrorout > 0)
                    DC[dc].apply_upload_wan += DATA_UNIT;

                for (auto &out_neighbour : *vertex[v].out_edge)
                {
                    if (vertex[out_neighbour].is_high_degree)
                    {
                        mirrorout--;
                        if (mirrorout == 0)
                        {
                            DC[origin_dc].apply_download_wan -= DATA_UNIT;
                            DC[dc].apply_upload_wan -= DATA_UNIT;
                        }

                        if (vertex[out_neighbour].current_dc == origin_dc)
                        {
                            // vertex[out_neighbour].local_in_degree--;

                            if (mirror[dc][out_neighbour].in_degree == 0)
                            {
                                DC[dc].gather_upload_wan += DATA_UNIT;
                                DC[origin_dc].gather_download_wan += DATA_UNIT;
                            }
                            // mirror[dc][out_neighbour].add(1, 0);
                        }
                        else
                        {
                            int out_neighbour_dc = vertex[out_neighbour].current_dc;
                            // mirror[origin_dc][out_neighbour].local_in_degree--;
                            if (mirror[origin_dc][out_neighbour].in_degree == 1)
                            {
                                DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                                DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
                            }
                            if (out_neighbour_dc != dc)

                            {
                                if (mirror[dc][out_neighbour].in_degree == 0)
                                {
                                    DC[dc].gather_upload_wan += DATA_UNIT;
                                    DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
                                }
                                // mirror[dc][out_neighbour].add(1, 0);
                            }
                        }
                    }
                }
            }
            else
            {
                // if (vertex[v].id == 12) // 12
                // {

                //     // printf("=== SCORE === : dc %d, score = %f, %f %f %f %f %f %f\n", dc, score[dc], graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost);
                //     for (auto &i : DC)
                //     {
                //         printf("DC : gu %f\tgd %f\tau %f\tad %f\n", i.gather_upload_wan, i.gather_download_wan, i.apply_upload_wan, i.apply_download_wan);
                //     }
                // }
                for (int i = 0; i < DC_num; i++)
                {
                    if (i == origin_dc)
                        continue;
                    if (mirror[i][v].out_degree > 0)
                        DC[origin_dc].apply_upload_wan -= DATA_UNIT;
                }

                if (mirror[origin_dc][v].in_degree > 0)
                    DC[origin_dc].gather_upload_wan += DATA_UNIT,
                        DC[dc].gather_download_wan += DATA_UNIT;
                if (mirror[origin_dc][v].out_degree > 0)
                    DC[origin_dc].apply_download_wan += DATA_UNIT,
                        DC[dc].apply_upload_wan += DATA_UNIT;

                if (mirror[dc][v].out_degree > 0)
                    DC[dc].apply_download_wan -= DATA_UNIT;

                int mirrorin = mirror[origin_dc][v].in_degree;
                int mirrorout = mirror[origin_dc][v].out_degree;
                // mirror[dc][v].in_use = false;
                // mirror[dc][v].del();

                for (int i = 0; i < DC_num; i++)
                {
                    if (i != dc && i != origin_dc && mirror[i][v].out_degree > 0)
                        DC[dc].apply_upload_wan += DATA_UNIT;
                }

                // if (vertex[v].id == 12) // 12
                // {

                //     // printf("=== SCORE === : dc %d, score = %f, %f %f %f %f %f %f\n", dc, score[dc], graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost);
                //     for (auto &i : DC)
                //     {
                //         printf("DC : gu %f\tgd %f\tau %f\tad %f\n", i.gather_upload_wan, i.gather_download_wan, i.apply_upload_wan, i.apply_download_wan);
                //     }
                //     printf("%d Mirror : ", vertex[v].id);
                //     for (int i = 0; i < DC_num; i++)
                //         printf("%d %d\t", mirror[i][v].in_degree, mirror[i][v].out_degree);
                //     printf("\n");
                // }

                for (auto &in_neighbour : vertex[v].in_edge)
                {
                    // mirror[origin_dc][v].local_in_degree--;
                    // vertex[v].local_in_degree++;
                    // if (vertex[v].id == 12) // 12
                    // {

                    //     printf("%d Mirror : ", vertex[in_neighbour].id);
                    //     for (int i = 0; i < DC_num; i++)
                    //         printf("%d %d\t", mirror[i][in_neighbour].in_degree, mirror[i][in_neighbour].out_degree);
                    //     printf("\n");
                    // }

                    mirrorin--;
                    if (mirrorin == 0)
                    {
                        DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                        DC[dc].gather_download_wan -= DATA_UNIT;
                    }
                    // if (vertex[in_neighbour].is_high_degree)
                    {
                        int in_neighbour_dc = vertex[in_neighbour].current_dc;
                        if (in_neighbour_dc == origin_dc)
                        {
                            // vertex[in_neighbour].local_out_degree--;
                            if (mirror[dc][in_neighbour].out_degree == 0)
                            {
                                DC[dc].apply_download_wan += DATA_UNIT;
                                DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
                            }
                            // mirror[dc][in_neighbour].add(0, 1);
                        }
                        else
                        {
                            // mirror[origin_dc][in_neighbour].local_out_degree--;
                            if (mirror[origin_dc][in_neighbour].out_degree == 1)
                            {
                                DC[origin_dc].apply_download_wan -= DATA_UNIT;
                                DC[in_neighbour_dc].apply_upload_wan -= DATA_UNIT;
                            }
                            if (in_neighbour_dc != dc)
                            {
                                if (mirror[dc][in_neighbour].out_degree == 0)
                                {
                                    DC[dc].apply_download_wan += DATA_UNIT;
                                    DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
                                }
                                // mirror[dc][in_neighbour].add(0, 1);
                            }
                        }
                    }
                }
                // if (vertex[v].id == 12) // 12
                // {

                //     // printf("=== SCORE === : dc %d, score = %f, %f %f %f %f %f %f\n", dc, score[dc], graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost);
                //     for (auto &i : DC)
                //     {
                //         printf("DC : gu %f\tgd %f\tau %f\tad %f\n", i.gather_upload_wan, i.gather_download_wan, i.apply_upload_wan, i.apply_download_wan);
                //     }
                // }
                for (auto &out_neighbour : *vertex[v].out_edge)
                {
                    if (vertex[out_neighbour].is_high_degree)
                    {
                        mirrorout--;
                        if (mirrorout == 0)
                        {
                            DC[origin_dc].apply_download_wan -= DATA_UNIT;
                            DC[dc].apply_upload_wan -= DATA_UNIT;
                        }

                        if (vertex[out_neighbour].current_dc == origin_dc)
                        {
                            // vertex[out_neighbour].local_in_degree--;

                            if (mirror[dc][out_neighbour].in_degree == 0)
                            {
                                DC[dc].gather_upload_wan += DATA_UNIT;
                                DC[origin_dc].gather_download_wan += DATA_UNIT;
                            }
                            // mirror[dc][out_neighbour].add(1, 0);
                        }
                        else
                        {
                            int out_neighbour_dc = vertex[out_neighbour].current_dc;
                            // mirror[origin_dc][out_neighbour].local_in_degree--;
                            if (mirror[origin_dc][out_neighbour].in_degree == 1)
                            {
                                DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                                DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
                            }
                            if (out_neighbour_dc != dc)

                            {
                                if (mirror[dc][out_neighbour].in_degree == 0)
                                {
                                    DC[dc].gather_upload_wan += DATA_UNIT;
                                    DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
                                }
                                // mirror[dc][out_neighbour].add(1, 0);
                            }
                        }
                    }
                }
            }
            double t, p;

            graph.calculate_network_time_price(t, p, DC);

            score[dc] = Score(graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost, it);
            // if (vertex[v].id == 12) // 12
            // {
            //     printf("=== SCORE === : dc %d, score = %.32f, %f %f %f %f %f %f\n", dc, score[dc], graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost);
            //     for (auto &i : DC)
            //     {
            //         printf("DC : gu %f\tgd %f\tau %f\tad %f\n", i.gather_upload_wan, i.gather_download_wan, i.apply_upload_wan, i.apply_download_wan);
            //     }
            // }
        }

        return score;
    }
    /*
        vector<double> moveVirtualVertex2(id_type v, int it)
        // 以当前环境为参考，移动顶点到所有dc，返回每个dc的score情况
        {
            int &DC_num = graph.DC_num;
            vector<Vertex> &vertex = graph.vertex;

            vector<double> score(DC_num);

            // unordered_map<id_type, int>

            for (int dc = 0; dc < DC_num; dc++)
            {
                if (vertex[v].current_dc == dc)
                    continue;
                double movecost = graph.movecost;
                vector<DataCenter> DC = graph.DC;

                int origin_dc = vertex[v].current_dc;
                int init_dc = vertex[v].init_dc;

                if (origin_dc == init_dc)
                {
                    movecost += MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
                }
                else if (dc == init_dc)
                {
                    movecost -= MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
                }
                if (vertex[v].is_high_degree)
                {
                    for (int i = 0; i < DC_num; i++)
                    {
                        if (i == vertex[v].current_dc)
                            continue;
                        if (mirror[i][v].in_degree > 0)
                            DC[origin_dc].gather_download_wan -= DATA_UNIT;
                        if (mirror[i][v].out_degree > 0)
                            DC[origin_dc].apply_upload_wan -= DATA_UNIT;
                    }
                    int mirrorin = vertex[v].mirror[vertex[v].current_dc].in_degree;
                    int mirrorout = vertex[v].mirror[vertex[v].current_dc].out_degree;

                    if (mirrorin > 0)
                        DC[origin_dc].gather_upload_wan += DATA_UNIT;
                    if (mirrorout > 0)
                        DC[origin_dc].apply_download_wan += DATA_UNIT;

                    if (mirror[dc][v].in_degree > 0)
                        DC[dc].gather_upload_wan -= DATA_UNIT;
                    if (mirror[dc][v].out_degree > 0)
                        DC[dc].apply_download_wan -= DATA_UNIT;

                    for (int i = 0; i < DC_num; i++)
                    {
                        if (i != dc && i != origin_dc && mirror[i][v].in_degree > 0)
                            DC[dc].gather_download_wan += DATA_UNIT;
                        if (i != dc && i != origin_dc && mirror[i][v].out_degree > 0)
                            DC[dc].apply_upload_wan += DATA_UNIT;
                    }

                    if (mirrorin > 0)
                        DC[dc].gather_download_wan += DATA_UNIT;
                    if (mirrorout > 0)
                        DC[dc].apply_upload_wan += DATA_UNIT;

                    for (auto &out_neighbour : vertex[v].out_edge)
                    {
                        if (vertex[out_neighbour].is_high_degree)
                        {
                            mirrorout--;
                            if (mirrorout == 0)
                            {
                                DC[origin_dc].apply_download_wan -= DATA_UNIT;
                                DC[dc].apply_upload_wan -= DATA_UNIT;
                            }

                            if (vertex[out_neighbour].current_dc == origin_dc)
                            {
                                // vertex[out_neighbour].local_in_degree--;

                                if (mirror[dc][out_neighbour].in_degree == 0)
                                {
                                    DC[dc].gather_upload_wan += DATA_UNIT;
                                    DC[origin_dc].gather_download_wan += DATA_UNIT;
                                }
                                // mirror[dc][out_neighbour].add(1, 0);
                            }
                            else
                            {
                                int out_neighbour_dc = vertex[out_neighbour].current_dc;
                                // mirror[origin_dc][out_neighbour].local_in_degree--;
                                if (mirror[origin_dc][out_neighbour].in_degree == 1)
                                {
                                    DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                                    DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
                                }
                                if (out_neighbour_dc != dc)

                                {
                                    if (mirror[dc][out_neighbour].in_degree == 0)
                                    {
                                        DC[dc].gather_upload_wan += DATA_UNIT;
                                        DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
                                    }
                                    // mirror[dc][out_neighbour].add(1, 0);
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < DC_num; i++)
                    {
                        if (i == vertex[v].current_dc)
                            continue;
                        if (mirror[i][v].out_degree > 0)
                            DC[origin_dc].apply_upload_wan -= DATA_UNIT;
                    }

                    if (vertex[v].mirror[vertex[v].current_dc].in_degree > 0)
                        DC[origin_dc].gather_upload_wan += DATA_UNIT,
                            DC[dc].gather_download_wan += DATA_UNIT;
                    if (vertex[v].mirror[vertex[v].current_dc].out_degree > 0)
                        DC[origin_dc].apply_download_wan += DATA_UNIT,
                            DC[dc].apply_upload_wan += DATA_UNIT;

                    // if (mirror[dc][v].in_use)
                    {
                        // if (mirror[dc][v].local_in_degree > 0)
                        // DC[dc].gather_upload_wan -= DATA_UNIT;
                        if (mirror[dc][v].out_degree > 0)
                            DC[dc].apply_download_wan -= DATA_UNIT;
                    }

                    int mirrorin = vertex[v].mirror[vertex[v].current_dc].in_degree;
                    int mirrorout = vertex[v].mirror[vertex[v].current_dc].out_degree;
                    // mirror[dc][v].in_use = false;
                    // mirror[dc][v].del();

                    for (int i = 0; i < DC_num; i++)
                    {
                        if (i != dc && i != origin_dc && mirror[i][v].out_degree > 0)
                            DC[dc].apply_upload_wan += DATA_UNIT;
                    }

                    for (auto &in_neighbour : vertex[v].in_edge)
                    {
                        // mirror[origin_dc][v].local_in_degree--;
                        // vertex[v].local_in_degree++;
                        mirrorin--;
                        if (mirrorin == 0)
                        {
                            DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                            DC[dc].gather_download_wan -= DATA_UNIT;
                        }
                        // if (vertex[in_neighbour].is_high_degree)
                        {
                            int in_neighbour_dc = vertex[in_neighbour].current_dc;
                            if (in_neighbour_dc == origin_dc)
                            {
                                // vertex[in_neighbour].local_out_degree--;
                                if (mirror[dc][in_neighbour].out_degree == 0)
                                {
                                    DC[dc].apply_download_wan += DATA_UNIT;
                                    DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
                                }
                                // mirror[dc][in_neighbour].add(0, 1);
                            }
                            else
                            {
                                // mirror[origin_dc][in_neighbour].local_out_degree--;
                                if (mirror[origin_dc][in_neighbour].out_degree == 1)
                                {
                                    DC[origin_dc].apply_download_wan -= DATA_UNIT;
                                    DC[in_neighbour_dc].apply_upload_wan -= DATA_UNIT;
                                }
                                if (in_neighbour_dc != dc)
                                {
                                    if (mirror[dc][in_neighbour].out_degree == 0)
                                    {
                                        DC[dc].apply_download_wan += DATA_UNIT;
                                        DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
                                    }
                                    // mirror[dc][in_neighbour].add(0, 1);
                                }
                            }
                        }
                    }
                    for (auto &out_neighbour : vertex[v].out_edge)
                    {
                        if (vertex[out_neighbour].is_high_degree)
                        {
                            mirrorout--;
                            if (mirrorout == 0)
                            {
                                DC[origin_dc].apply_download_wan -= DATA_UNIT;
                                DC[dc].apply_upload_wan -= DATA_UNIT;
                            }

                            if (vertex[out_neighbour].current_dc == origin_dc)
                            {
                                // vertex[out_neighbour].local_in_degree--;

                                if (mirror[dc][out_neighbour].in_degree == 0)
                                {
                                    DC[dc].gather_upload_wan += DATA_UNIT;
                                    DC[origin_dc].gather_download_wan += DATA_UNIT;
                                }
                                // mirror[dc][out_neighbour].add(1, 0);
                            }
                            else
                            {
                                int out_neighbour_dc = vertex[out_neighbour].current_dc;
                                // mirror[origin_dc][out_neighbour].local_in_degree--;
                                if (mirror[origin_dc][out_neighbour].in_degree == 1)
                                {
                                    DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                                    DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
                                }
                                if (out_neighbour_dc != dc)

                                {
                                    if (mirror[dc][out_neighbour].in_degree == 0)
                                    {
                                        DC[dc].gather_upload_wan += DATA_UNIT;
                                        DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
                                    }
                                    // mirror[dc][out_neighbour].add(1, 0);
                                }
                            }
                        }
                    }
                }
                double t, p;

                graph.calculate_network_time_price(t, p, DC);

                score[dc] = Score(graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost, it);
                // if(graph.vertex[v].id == 252)
                // {
                //     for(auto &x : DC)
                //     {
                //         printf("if move to DC %d gu %.32f gd %.32f au %.32f ad %.32f \n", dc, x.gather_upload_wan, x.gather_download_wan, x.apply_upload_wan, x.apply_download_wan);
                //     }
                //     printf("DC %d t: %.32f p: %.32f\n ", dc, t, p);
                // }
            }
            // if(graph.vertex[v].id == 252)
            //     {
            //         cout << "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" << endl;
            //         for(auto &ss : score)
            //             printf("%.32f ", ss);

            //         cout << endl ;
            //     }
            return score;
        }
        */

    double Score(double old_time, double old_cost, double old_mvcost, double new_time, double new_cost, double new_mvcost, int iter)
    // 计算得分
    {
        double score;
        // if (new_cost + new_mvcost < Budget)
        // score = (old_time - new_time) / old_time + (old_cost - new_cost) / old_cost;
        score = (old_time - new_time) / old_time;
        // else

        if (new_mvcost + new_cost > local_budget)
        {
            double old_ = old_cost + old_mvcost;
            double new_ = new_cost + new_mvcost;
            double b = 0.75;

            double rate = b * (1 + iter) / global_iteration;

            score = (b - rate) * score + (1 - b + rate) * (old_mvcost - new_mvcost) / old_mvcost;
        }

        return score;
    }

    void update_sampling_rate(int cur_it, vector<double> &x, double left_overhead)
    {

        if (!auto_sampling_rate)
            return;
        int max_len = 10;
        int min_len = min(cur_it + 1, max_len);
        double sum = 0;
        for (int i = cur_it + 1 - min_len; i < cur_it + 1; i++)
            sum += x[i];
        double left_time_per_iteration = left_overhead / (global_iteration - cur_it - 1);
        sampling_rate = max(0., sum / min_len * left_time_per_iteration);
        sampling_rate = min(1., sampling_rate);
    }

    /*
    void update_sampling_rate(int cur_it, vector<double> &x, double left_overhead)
    {
        if (cur_it == global_iteration - 1)
            return;
        double predict_overhead = left_overhead / (global_iteration - cur_it - 1) * global_iteration;
        vector<pair<int, double>> all_res;
        for (int l = 1;; l++)
        {
            vector<pair<int, double>> cur_res;
            for (double sp = 0.01; sp <= 1.; sp += 0.01)
            {
                // Overhead = 129.0837 * local * sp + 21.5339 * local + 205.0872 * sp + 103.9572
                if (129.0837 * l * sp + 21.5339 * l + 205.0872 * sp + 103.9572 >= predict_overhead)
                    break;
                cur_res.push_back({l, sp});
            }
            if (cur_res.size() == 0)
                break;
            all_res.insert(all_res.end(), cur_res.begin(), cur_res.end());
        }
        printf("client %d get %d res:\n", myid, all_res.size());
        // for(auto &x : all_res)  printf("<%d, %f>\t", x.first, x.second);
        // printf("\n=========\n");

        double best_DTT = 1e20;
        int best_local = 0;
        double best_sp = 0;

        for (auto &x : all_res)
        {
            double tmp = func(x.first, x.second);
            if (tmp < best_DTT)
            {
                best_DTT = tmp;
                best_local = x.first;
                best_sp = x.second;
            }
        }
        printf("client %d try %d local iteration, %f sampling rate\n", myid, best_local, best_sp);

        sampling_rate = best_sp;
        local_iteration = best_local;
    }
    inline double func(int local, double sp)
    {
        if (sp <= 0.2)
            return -0.1473 * local * sp + -0.0145 * local + -0.5436 * sp + 1.1513;
        else if (sp <= 0.4)
            return 0.0313 * local * sp + -0.0502 * local + -0.5709 * sp + 1.1567;
        else if (sp <= 0.8)
            return 0.0716 * local * sp + -0.0664 * local + -0.6170 * sp + 1.1752;
        else
            return 0.0269 * local * sp + -0.0306 * local + -0.2626 * sp + 0.8917;
    }*/

    int last_explore_action = 0; // 0: sampling rate, 1: local iteration num
    double last_sampling_rate = default_sampling_rate;
    int last_local_iteration_num = local_iteration;
    // void update_sampling_rate(int cur_it, double worker_it_overhead, double global_it_overhead, double left_overhead, double last_DTT, double cur_DTT)
    void update_sampling_rate(int cur_it, double worker_it_overhead, double global_it_overhead, double left_overhead, double cur, double last)
    {
        // Load balance in worker
        sampling_rate *= global_it_overhead / worker_it_overhead;
        sampling_rate = min(1., sampling_rate);
        sampling_rate = max(0., sampling_rate);
        printf("[Load balance] Worker %d change sampling rate to %f\n", myid, sampling_rate);

        bool ban_sampling_rate = false;
        bool ban_local = false;

        if (cur >= last)
        {
            if (last_explore_action == 0)
            {
                sampling_rate = last_sampling_rate;
                ban_sampling_rate = true;
                printf("[Bad DTT] Worker %d change sampling rate to %f\n", myid, sampling_rate);
            }
            else
            {
                local_iteration = last_local_iteration_num;
                ban_local = true;
                printf("[Bad DTT] Worker %d change local iteration num to %d\n", myid, local_iteration);
            }
        }

        double avg_overhead = left_overhead / (global_iteration - cur_it - 1);
        if (global_it_overhead >= avg_overhead)
        {
            sampling_rate *= avg_overhead / global_it_overhead;

            sampling_rate = min(1., sampling_rate);
            sampling_rate = max(0., sampling_rate);
            printf("[Overhead excessive] Worker %d change sampling rate num to %f\n", myid, sampling_rate);
        }
        else
        {
            double a = 1. + 1. / (cur_it + 1);
            if (global_it_overhead * a >= avg_overhead)
            {
                sampling_rate *= avg_overhead / global_it_overhead;

                sampling_rate = min(1., sampling_rate);
                sampling_rate = max(0., sampling_rate);
                printf("[Not enough ovehead to explore] Worker %d change sampling rate num to %f\n", myid, sampling_rate);
                printf("[Not enough ovehead to explore] Worker %d %f %f\n", myid, global_it_overhead * a, avg_overhead);
            }
            else
            {
                if (ban_local || rand() % 2 == 0 && ban_sampling_rate == false)
                {
                    last_sampling_rate = sampling_rate;
                    sampling_rate *= a;

                    sampling_rate = min(1., sampling_rate);
                    sampling_rate = max(0., sampling_rate);
                    last_explore_action = 0;
                    printf("[Explore sampling rate] Worker %d change sampling rate num to %f\n", myid, sampling_rate);
                }
                else if (ban_local == false)
                {
                    last_local_iteration_num = local_iteration;
                    // local_iteration = ceil(local_iteration * a);
                    local_iteration += 1;
                    last_explore_action = 1;
                    printf("[Explore local iteration num] Worker %d change local iteration num to %d\n", myid, local_iteration);
                }
            }
        }
        printf("Wokrer %d local = %d sampling rate = %f\n", myid, local_iteration, sampling_rate);
    }
};

#endif