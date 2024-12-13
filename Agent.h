#include "Graph_dis_mem.h"
#include <bits/stdc++.h>
#include <omp.h>
#include <mpi.h>

#ifndef __AGENT__
#define __AGENT__

extern double train_rate;
extern double Budget_rate;
extern bool auto_sampling_rate;
extern double default_sampling_rate;
extern double overhead_limit;
extern int block_nums;
extern int cut_num;
extern int vertex_num_in_agent;
extern int client_num;
extern double overhead_limit;
extern int switch_iteration;

extern int myid;
extern int numprocs;

extern int local_iteration;

// enum MPI_TAG
// {
//     SERVER_SEND_BUDGET2CLIENTS,
//     SERVER_SEND_AGENTS_NUM2CLIENTS,
//     SERVER_DISTRIBUTE_AGENT2CLIENTS,
//     SERVER_SEND_BUDGET2CLIENT

// };

// class MACHINE
// {
// public:
//     Graph graph;
//     // vector<Vertex> agent_vector;
//     id_type agent_num;
//     vector<id_type> agent_vector;
//     time_t now;

//     double alpha = 0.4; // Alpha参数
//     // double Budget_rate = 0.2; // Budget占的比例
//     MACHINE(){};
//     struct agent_move
//     {
//         id_type v;
//         int origin_dc;
//         int choice_dc;
//     };
//     MACHINE(Graph g)
//     {
//         graph = g;
//     }
//     MACHINE(string graph_file_name, string network_file_name, bool display = false) : graph(graph_file_name, network_file_name)
//     {
//         graph.display_graph_state = display;
//         graph.read_file();
//         // graph.random_partition_rate();
//         graph.average_partition();
//         graph.init_para();
//         graph.hybrid_cut_parallel();
//         // graph.hybrid_cut();
//         // graph.print();
//         graph.calculate_network_wan();
//     }
//     string get_time()
//     {
//         now = time(0);
//         tm *ltm = localtime(&now);
//         return (to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min) + ":" + to_string(ltm->tm_sec));
//     }
// };

// class CLIENT : public MACHINE
// {
// public:
//     double local_budget;
//     int local_iteration = 10; // 迭代次数
//     int global_iteration = 10;
//     double sampling_rate;

//     CLIENT(string graph_file_name, string network_file_name, int global_it, int lo) : MACHINE(graph_file_name, network_file_name)
//     {
//         local_iteration = lo;
//         global_iteration = global_it;
//         sampling_rate = default_sampling_rate;
//     }

//     vector<int> pre_choice;

//     // vector<agent_move> local_change;
//     unordered_map<id_type, agent_move> local_change;
//     vector<uint> local_change_v;

//     vector<vector<double>> probability; // 概率
//     void init()
//     {
//         probability = vector<vector<double>>(agent_vector.size(), vector<double>(graph.DC_num, 1. / graph.DC_num));
//         pre_choice = vector<int>(agent_vector.size(), -1);
//         sort(agent_vector.begin(), agent_vector.end(), [&](id_type a, id_type b)
//              { return graph.vertex[a].in_degree > graph.vertex[b].in_degree; });
//     }
//     double Score(double old_time, double old_cost, double old_mvcost, double new_time, double new_cost, double new_mvcost, int iter)
//     // 计算得分
//     {
//         double score;
//         // if (new_cost + new_mvcost < Budget)
//         score = (old_time - new_time) / old_time + (old_cost - new_cost) / old_cost;
//         // else

//         if (new_mvcost > local_budget)
//         {
//             double old_ = old_cost + old_mvcost;
//             double new_ = new_cost + new_mvcost;
//             double b = 0.75;

//             double rate = b * (1 + iter) / local_iteration;

//             score = (b - rate) * score + (1 - b + rate) * (old_mvcost - new_mvcost) / old_mvcost;
//         }

//         return score;
//     }
//     vector<double> moveVirtualVertex(id_type v, int it)
//     // 以当前环境为参考，移动顶点到所有dc，返回每个dc的score情况
//     {
//         int &DC_num = graph.DC_num;
//         vector<Vertex> &vertex = graph.vertex;

//         vector<std::vector<Mirror>> &mirror = graph.mirror;

//         vector<double> score(DC_num);

//         // unordered_map<id_type, int>

//         for (int dc = 0; dc < DC_num; dc++)
//         {
//             if (vertex[v].current_dc == dc)
//                 continue;
//             double movecost = graph.movecost;
//             vector<DataCenter> DC = graph.DC;

//             int origin_dc = vertex[v].current_dc;
//             int init_dc = vertex[v].init_dc;

//             if (origin_dc == init_dc)
//             {
//                 movecost += MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
//             }
//             else if (dc == init_dc)
//             {
//                 movecost -= MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
//             }
//             if (vertex[v].is_high_degree)
//             {
//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (mirror[i][v].in_use && mirror[i][v].local_in_degree > 0)
//                         DC[origin_dc].gather_download_wan -= DATA_UNIT;
//                     if (mirror[i][v].in_use && mirror[i][v].local_out_degree > 0)
//                         DC[origin_dc].apply_upload_wan -= DATA_UNIT;
//                 }
//                 int mirrorin = vertex[v].local_in_degree;
//                 int mirrorout = vertex[v].local_out_degree;

//                 if (mirrorin > 0)
//                     DC[origin_dc].gather_upload_wan += DATA_UNIT;
//                 if (mirrorout > 0)
//                     DC[origin_dc].apply_download_wan += DATA_UNIT;

//                 if (mirror[dc][v].local_in_degree > 0)
//                     DC[dc].gather_upload_wan -= DATA_UNIT;
//                 if (mirror[dc][v].local_out_degree > 0)
//                     DC[dc].apply_download_wan -= DATA_UNIT;

//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (i != dc && i != origin_dc && mirror[i][v].local_in_degree > 0)
//                         DC[dc].gather_download_wan += DATA_UNIT;
//                     if (i != dc && i != origin_dc && mirror[i][v].local_out_degree > 0)
//                         DC[dc].apply_upload_wan += DATA_UNIT;
//                 }

//                 if (mirrorin > 0)
//                     DC[dc].gather_download_wan += DATA_UNIT;
//                 if (mirrorout > 0)
//                     DC[dc].apply_upload_wan += DATA_UNIT;

//                 for (auto &out_neighbour : vertex[v].out_edge)
//                 {
//                     if (vertex[out_neighbour].is_high_degree)
//                     {
//                         mirrorout--;
//                         if (mirrorout == 0)
//                         {
//                             DC[origin_dc].apply_download_wan -= DATA_UNIT;
//                             DC[dc].apply_upload_wan -= DATA_UNIT;
//                         }

//                         if (vertex[out_neighbour].current_dc == origin_dc)
//                         {
//                             // vertex[out_neighbour].local_in_degree--;

//                             if (mirror[dc][out_neighbour].local_in_degree == 0)
//                             {
//                                 DC[dc].gather_upload_wan += DATA_UNIT;
//                                 DC[origin_dc].gather_download_wan += DATA_UNIT;
//                             }
//                             // mirror[dc][out_neighbour].add(1, 0);
//                         }
//                         else
//                         {
//                             int out_neighbour_dc = vertex[out_neighbour].current_dc;
//                             // mirror[origin_dc][out_neighbour].local_in_degree--;
//                             if (mirror[origin_dc][out_neighbour].local_in_degree == 1)
//                             {
//                                 DC[origin_dc].gather_upload_wan -= DATA_UNIT;
//                                 DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
//                             }
//                             if (out_neighbour_dc != dc)

//                             {
//                                 if (mirror[dc][out_neighbour].local_in_degree == 0)
//                                 {
//                                     DC[dc].gather_upload_wan += DATA_UNIT;
//                                     DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
//                                 }
//                                 // mirror[dc][out_neighbour].add(1, 0);
//                             }
//                         }
//                     }
//                 }
//             }
//             else
//             {
//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (mirror[i][v].in_use && mirror[i][v].local_out_degree > 0)
//                         DC[origin_dc].apply_upload_wan -= DATA_UNIT;
//                 }

//                 if (vertex[v].local_in_degree > 0)
//                     DC[origin_dc].gather_upload_wan += DATA_UNIT,
//                         DC[dc].gather_download_wan += DATA_UNIT;
//                 if (vertex[v].local_out_degree > 0)
//                     DC[origin_dc].apply_download_wan += DATA_UNIT,
//                         DC[dc].apply_upload_wan += DATA_UNIT;

//                 if (mirror[dc][v].in_use)
//                 {
//                     // if (mirror[dc][v].local_in_degree > 0)
//                     // DC[dc].gather_upload_wan -= DATA_UNIT;
//                     if (mirror[dc][v].local_out_degree > 0)
//                         DC[dc].apply_download_wan -= DATA_UNIT;
//                 }

//                 int mirrorin = vertex[v].local_in_degree;
//                 int mirrorout = vertex[v].local_out_degree;
//                 // mirror[dc][v].in_use = false;
//                 // mirror[dc][v].del();

//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (i != dc && i != origin_dc && mirror[i][v].local_out_degree > 0)
//                         DC[dc].apply_upload_wan += DATA_UNIT;
//                 }

//                 for (auto &in_neighbour : vertex[v].in_edge)
//                 {
//                     // mirror[origin_dc][v].local_in_degree--;
//                     // vertex[v].local_in_degree++;
//                     mirrorin--;
//                     if (mirrorin == 0)
//                     {
//                         DC[origin_dc].gather_upload_wan -= DATA_UNIT;
//                         DC[dc].gather_download_wan -= DATA_UNIT;
//                     }
//                     // if (vertex[in_neighbour].is_high_degree)
//                     {
//                         int in_neighbour_dc = vertex[in_neighbour].current_dc;
//                         if (in_neighbour_dc == origin_dc)
//                         {
//                             // vertex[in_neighbour].local_out_degree--;
//                             if (mirror[dc][in_neighbour].local_out_degree == 0)
//                             {
//                                 DC[dc].apply_download_wan += DATA_UNIT;
//                                 DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
//                             }
//                             // mirror[dc][in_neighbour].add(0, 1);
//                         }
//                         else
//                         {
//                             // mirror[origin_dc][in_neighbour].local_out_degree--;
//                             if (mirror[origin_dc][in_neighbour].local_out_degree == 1)
//                             {
//                                 DC[origin_dc].apply_download_wan -= DATA_UNIT;
//                                 DC[in_neighbour_dc].apply_upload_wan -= DATA_UNIT;
//                             }
//                             if (in_neighbour_dc != dc)
//                             {
//                                 if (mirror[dc][in_neighbour].local_out_degree == 0)
//                                 {
//                                     DC[dc].apply_download_wan += DATA_UNIT;
//                                     DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
//                                 }
//                                 // mirror[dc][in_neighbour].add(0, 1);
//                             }
//                         }
//                     }
//                 }
//                 for (auto &out_neighbour : vertex[v].out_edge)
//                 {
//                     if (vertex[out_neighbour].is_high_degree)
//                     {
//                         mirrorout--;
//                         if (mirrorout == 0)
//                         {
//                             DC[origin_dc].apply_download_wan -= DATA_UNIT;
//                             DC[dc].apply_upload_wan -= DATA_UNIT;
//                         }

//                         if (vertex[out_neighbour].current_dc == origin_dc)
//                         {
//                             // vertex[out_neighbour].local_in_degree--;

//                             if (mirror[dc][out_neighbour].local_in_degree == 0)
//                             {
//                                 DC[dc].gather_upload_wan += DATA_UNIT;
//                                 DC[origin_dc].gather_download_wan += DATA_UNIT;
//                             }
//                             // mirror[dc][out_neighbour].add(1, 0);
//                         }
//                         else
//                         {
//                             int out_neighbour_dc = vertex[out_neighbour].current_dc;
//                             // mirror[origin_dc][out_neighbour].local_in_degree--;
//                             if (mirror[origin_dc][out_neighbour].local_in_degree == 1)
//                             {
//                                 DC[origin_dc].gather_upload_wan -= DATA_UNIT;
//                                 DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
//                             }
//                             if (out_neighbour_dc != dc)

//                             {
//                                 if (mirror[dc][out_neighbour].local_in_degree == 0)
//                                 {
//                                     DC[dc].gather_upload_wan += DATA_UNIT;
//                                     DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
//                                 }
//                                 // mirror[dc][out_neighbour].add(1, 0);
//                             }
//                         }
//                     }
//                 }
//             }
//             double t, p;

//             graph.calculate_network_time_price(t, p, DC);

//             score[dc] = Score(graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost, it);
//         }

//         return score;
//     }
//     int make_decision_greedy(id_type agent_index)
//     {
//         int choice_dc = max_element(probability[agent_index].begin(), probability[agent_index].end()) - probability[agent_index].begin();
//         return choice_dc;
//     }
//     void sampling_update_prob_and_make_greedy_choice(int it)
//     {
//         // cout << "---" << client_thread_num << endl;
// #pragma omp parallel for
//         // for (id_type vv = 0; vv < client_thread_num; vv++)
//         for (id_type vv = 0; vv < agent_vector.size(); vv++)
//         {

//             int sp_rate = sampling_rate * RAND_MAX;
//             if (rand() < sp_rate)
//             {
//                 id_type v = agent_vector[vv];
//                 vector<double> s = moveVirtualVertex(v, it);
//                 int max_s = max_element(s.begin(), s.end()) - s.begin();
//                 for (int dc = 0; dc < graph.DC_num; dc++)
//                 {
//                     if (max_s == dc)
//                         probability[vv][dc] += alpha * (1 - probability[vv][dc]);
//                     else
//                         probability[vv][dc] = (1 - alpha) * probability[vv][dc];
//                 }
//                 pre_choice[vv] = make_decision_greedy(vv);
//             }
//             else
//                 pre_choice[vv] = -1;
//         }
//     }
//     void topK_sampling_update_prob_and_make_greedy_choice(int global_it)
//     {
//         // cout << "---" << client_thread_num << endl;
//         // memset(pre_choice, -1, sizeof(pre_choice));
//         const int agents_per_it = agent_vector.size() / global_iteration;
//         printf("%d~%d\n", global_it * agents_per_it, (global_it + 1) * agents_per_it);
// #pragma omp parallel for
//         // for (id_type vv = 0; vv < client_thread_num; vv++)
//         for (id_type vv = global_it * agents_per_it; vv < (global_it + 1) * agents_per_it; vv++)
//         {
//             id_type v = agent_vector[vv];
//             vector<double> s = moveVirtualVertex(v, 0); // local iteration 默认为1了
//             int max_s = max_element(s.begin(), s.end()) - s.begin();
//             for (int dc = 0; dc < graph.DC_num; dc++)
//             {
//                 if (max_s == dc)
//                     probability[vv][dc] += alpha * (1 - probability[vv][dc]);
//                 else
//                     probability[vv][dc] = (1 - alpha) * probability[vv][dc];
//             }
//             pre_choice[vv] = make_decision_greedy(vv);
//         }
//     }
//     void update_prob_and_make_greedy_choice(int it)
//     {
//         // cout << "---" << client_thread_num << endl;
// #pragma omp parallel for
//         // for (id_type vv = 0; vv < client_thread_num; vv++)
//         for (id_type vv = 0; vv < agent_vector.size(); vv++)
//         {

//             id_type v = agent_vector[vv];
//             vector<double> s = moveVirtualVertex(v, it);
//             int max_s = max_element(s.begin(), s.end()) - s.begin();
//             for (int dc = 0; dc < graph.DC_num; dc++)
//             {
//                 if (max_s == dc)
//                     probability[vv][dc] += alpha * (1 - probability[vv][dc]);
//                 else
//                     probability[vv][dc] = (1 - alpha) * probability[vv][dc];
//             }
//             pre_choice[vv] = make_decision_greedy(vv);
//         }
//     }
//     void train(int global_it)
//     {
//         // printf("Local training using thread %d \n", omp_get_thread_num());
//         // local_iteration = 1;
//         local_change.clear();
//         local_change_v.clear();
//         for (int it = 0; it < local_iteration; it++)
//         {
//             if (auto_sampling_rate)
//                 sampling_update_prob_and_make_greedy_choice(it);
//             else
//                 update_prob_and_make_greedy_choice(it);

//             for (id_type i = 0; i < agent_vector.size(); i++)
//             {
//                 id_type v = agent_vector[i];
//                 int origin_dc = graph.vertex[v].current_dc;
//                 double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                 int choice_dc = pre_choice[i];
//                 if (choice_dc == -1)
//                     continue;
//                 graph.moveVertex(v, choice_dc);
//                 double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;

//                 if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, it) > 0)
//                 {
//                     for (int dc = 0; dc < graph.DC_num; dc++)
//                     {
//                         if (choice_dc == dc)
//                             probability[i][dc] += alpha * (1 - probability[i][dc]);
//                         else
//                             probability[i][dc] = (1 - alpha) * probability[i][dc];
//                     }
//                     if (local_change.count(v))
//                     {
//                         if (choice_dc == local_change[v].origin_dc)
//                             local_change.erase(v);
//                         else
//                             local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     }
//                     else
//                         local_change[v] = {v, origin_dc, choice_dc};
//                     // if (local_change.count(v))
//                     //     local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     // else
//                     //     local_change[v] = {v, origin_dc, choice_dc};
//                 }
//                 else
//                 {
//                     graph.moveVertex(v, origin_dc);
//                 }
//             }
//         }
//         local_change_v.reserve(local_change.size() * 2);
//         for (auto &x : local_change)
//             local_change_v.emplace_back(x.first), local_change_v.emplace_back(x.second.choice_dc);

//         // cout << "------------------------" << endl;
//         // graph.print();

//         // cout << "------------------------" << endl;
//     }
//     void topk_train(int global_it)
//     {
//         // printf("Local training using thread %d \n", omp_get_thread_num());
//         // local_iteration = 1;
//         local_change.clear();
//         local_change_v.clear();
//         assert(local_iteration == 1);
//         for (int it = 0; it < local_iteration; it++)
//         {
//             if (auto_sampling_rate)
//                 topK_sampling_update_prob_and_make_greedy_choice(global_it);
//             // sampling_update_prob_and_make_greedy_choice(it);
//             else
//                 update_prob_and_make_greedy_choice(it);

//             const int agents_per_it = agent_vector.size() / global_iteration;
//             for (id_type i = global_it * agents_per_it; i < (global_it + 1) * agents_per_it; i++)
//             {
//                 id_type v = agent_vector[i];
//                 int origin_dc = graph.vertex[v].current_dc;
//                 double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                 int choice_dc = pre_choice[i];

//                 graph.moveVertex(v, choice_dc);
//                 double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;

//                 if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, it) > 0)
//                 {
//                     for (int dc = 0; dc < graph.DC_num; dc++)
//                     {
//                         if (choice_dc == dc)
//                             probability[i][dc] += alpha * (1 - probability[i][dc]);
//                         else
//                             probability[i][dc] = (1 - alpha) * probability[i][dc];
//                     }
//                     if (local_change.count(v))
//                     {
//                         if (choice_dc == local_change[v].origin_dc)
//                             local_change.erase(v);
//                         else
//                             local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     }
//                     else
//                         local_change[v] = {v, origin_dc, choice_dc};
//                     // if (local_change.count(v))
//                     //     local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     // else
//                     //     local_change[v] = {v, origin_dc, choice_dc};
//                 }
//                 else
//                 {
//                     graph.moveVertex(v, origin_dc);
//                 }
//             }
//         }
//         local_change_v.reserve(local_change.size() * 2);
//         for (auto &x : local_change)
//             local_change_v.emplace_back(x.first), local_change_v.emplace_back(x.second.choice_dc);

//         // cout << "------------------------" << endl;
//         // graph.print();

//         // cout << "------------------------" << endl;
//     }
//     void update_sampling_rate(int cur_it, vector<double> &x, double left_overhead)
//     {
//         if (!auto_sampling_rate)
//             return;
//         int max_len = 10;
//         int min_len = min(cur_it + 1, max_len);
//         double sum = 0;
//         for (int i = cur_it + 1 - min_len; i < cur_it + 1; i++)
//             sum += x[i];
//         double left_time_per_iteration = left_overhead / (global_iteration - cur_it - 1);
//         sampling_rate = max(0., sum / min_len * left_time_per_iteration);
//         sampling_rate = min(1., sampling_rate);
//     }
//     void train()
//     {
//         // if init finished
//         printf("[%s] Client %d --> waiting...\n", get_time().c_str(), myid);
//         MPI_Barrier(MPI_COMM_WORLD);
//         MPI_Recv(&agent_num, 1, MPI_UINT32_T, 0, SERVER_SEND_AGENTS_NUM2CLIENTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//         printf("[%s] Client %d --> Received %d agents num.\n", get_time().c_str(), myid, agent_num);
//         agent_vector.resize(agent_num);
//         MPI_Recv(agent_vector.data(), agent_num, MPI_UINT32_T, 0, SERVER_DISTRIBUTE_AGENT2CLIENTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//         printf("[%s] Client %d --> Received %ld agents finished.\n", get_time().c_str(), myid, agent_vector.size());
//         init();
//         MPI_Bcast(&local_budget, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
//         printf("[%s] Client %d --> Received %f local budget.\n", get_time().c_str(), myid, local_budget);
//         MPI_Barrier(MPI_COMM_WORLD);

//         auto train_begin = chrono::steady_clock::now();
//         uint change_sum;
//         vector<uint> apply_change;

//         vector<double> sampling_rate_per_iteration(global_iteration);
//         vector<double> overhead_per_iteration(global_iteration);
//         vector<double> samplingrate_overhead_rate(global_iteration);
//         double left_overhead = overhead_limit;
//         double overhead_use = 0;

//         for (int global = 0; global < global_iteration; global++)
//         {
//             auto iteration_begin = chrono::steady_clock::now();
//             auto client_train_begin = chrono::steady_clock::now();
//             train(global);
//             auto client_train_end = chrono::steady_clock::now();

//             // topk_train(global);
//             printf("[%s] Client %d --> Training finished\n", get_time().c_str(), myid);
//             id_type local_change_size = local_change.size() * 2;
//             MPI_Gather(&local_change_size, 1, MPI_UINT32_T, NULL, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
//             // printf("client %d send first %u->%d\n", myid, local_change_v[0], local_change_v[1]);
            
//             MPI_Gatherv(local_change_v.data(), local_change_v.size(),
//                         MPI_UINT32_T, NULL, NULL, NULL, MPI_UINT32_T, 0, MPI_COMM_WORLD);
//             if (global < global_iteration - 1)
//             {

//                 auto server_train_begin = chrono::steady_clock::now();
//                 apply_change.clear();
//                 MPI_Bcast(&change_sum, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
//                 apply_change.resize(change_sum);
//                 printf("[%s] Client %d --> Received %d change num.\n", get_time().c_str(), myid, change_sum);
//                 // MPI_Barrier(MPI_COMM_WORLD);
//                 MPI_Bcast(apply_change.data(), change_sum, MPI_UINT32_T, 0, MPI_COMM_WORLD);

                
//                 for (uint i = 0; i < apply_change.size(); i += 2)
//                     graph.moveVertex(apply_change[i], apply_change[i + 1]);
//                 auto server_train_end = chrono::steady_clock::now();

//                 auto iteration_end = chrono::steady_clock::now();
//                 double iteration_use_overhead = (double)std::chrono::duration_cast<std::chrono::milliseconds>(iteration_end - iteration_begin).count() / 1000;
//                 overhead_per_iteration[global] = iteration_use_overhead;
//                 overhead_use += iteration_use_overhead;
//                 samplingrate_overhead_rate[global] = sampling_rate / iteration_use_overhead;

//                 auto training_end = chrono::steady_clock::now();
//                 left_overhead = overhead_limit - ((double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - train_begin).count() / 1000);
//                 update_sampling_rate(global, samplingrate_overhead_rate, left_overhead);
//                 printf("[%s] Client %d update sampling rate to %f.\n", get_time().c_str(), myid, sampling_rate);

//                 MPI_Bcast(&graph.movecost, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
//                 printf("[%s] Client %d --> Received %f local mvcost.\n", get_time().c_str(), myid, graph.movecost);
//                 printf("[%s] Client %d --> waiting...\n", get_time().c_str(), myid);
//                 MPI_Barrier(MPI_COMM_WORLD);
//             }
//         }
//     }
// };

// class SERVER : public MACHINE
// {
// public:
//     int global_iteration = 10; // 迭代次数
//     double Budget;             // Budget预算

//     double origin_time; // 初始通信时间
//     double origin_cost; // 初始开销
//     double sampling_rate;

//     ofstream log_file;
//     SERVER(string graph_file_name, string network_file_name) : MACHINE(graph_file_name, network_file_name, true)
//     {
//         graph.display_graph_state = true;
//         sampling_rate = default_sampling_rate;
//         graph.print();

//         // graph = g;
//         log_file.open("LOG.txt", ios::out | ios::trunc);
//         if (!log_file.is_open())
//         {
//             cout << "can't open LOG file" << endl;
//             exit(-1);
//         }
//         log_file.setf(ios::fixed);
//         log_file.precision(5);

//         log_file << "Graph file name: " << graph.GRAPH_FILE_NAME << endl;
//         log_file << "Network file name: " << graph.NETWORK_FILE_NAME << endl;
//         log_file << "Client num: " << numprocs - 1 << endl;
//         log_file << "Server client mode" << endl; 
//     }
//     ~SERVER()
//     {
//         log_file.close();
//     }
//     void set_global_iteration(int t)
//     {
//         global_iteration = t;
//     }
//     vector<vector<id_type>> random_distribute_agent_to_machine()
//     {
//         int machine_num = numprocs - 1;
//         vector<int> which_machine(graph.vertex_num);
// #pragma omp parallel for
//         for (id_type i = 0; i < graph.vertex_num; i++)
//             which_machine[i] = rand() % machine_num;

//         vector<vector<id_type>> client_agents(machine_num);
// #pragma omp parallel for num_threads(machine_num)
//         for (int i = 0; i < machine_num; i++)
//             for (id_type j = 0; j < graph.vertex_num; j++)
//                 if (which_machine[j] == i)
//                     client_agents[i].push_back(j);

//         // cout << "Finished agent distribution:" << endl;
//         log_file << "Using Random distribute agent." << endl;

//         for (int i = 0; i < machine_num; i++)
//         {
//             reverse(client_agents[i].begin(), client_agents[i].end());
//             printf("Machine[%d] : %lu\tangents.\n", i, client_agents[i].size());
//         }
//         return client_agents;
//     }
//     void count_budget()
//     {
//         int max_price_dc = 0;                       // 记录最贵的服务器
//         double max_price = graph.DC[0].UploadPrice; // 记录最贵的服务器上传价格
//         for (int i = 0; i < graph.DC_num; i++)
//         {
//             if (max_price < graph.DC[i].UploadPrice)
//                 max_price_dc = i, max_price = graph.DC[i].UploadPrice;
//             Budget += graph.DC[i].vertex_num * graph.DC[i].UploadPrice;
//         }
//         Budget -= graph.DC[max_price_dc].vertex_num * graph.DC[max_price_dc].UploadPrice;
//         Budget *= Budget_rate;
//         Budget *= MOVE_DATA_UNIT; // Budget为将所有顶点迁移到最贵的DC所消耗的价格的budget_rate
//         origin_time = graph.transfer_time;
//         origin_cost = graph.transfer_cost;
//     }
//     double Score(double old_time, double old_cost, double old_mvcost, double new_time, double new_cost, double new_mvcost, int iter)
//     // 计算得分
//     {
//         double score;
//         // if (new_cost + new_mvcost < Budget)
//         score = (old_time - new_time) / old_time + (old_cost - new_cost) / old_cost;
//         // else

//         // 这是server的score，因为第一轮old mvcost是0，需要排除
//         if (new_mvcost > Budget)
//         {
//             double old_ = old_cost + old_mvcost;
//             double new_ = new_cost + new_mvcost;
//             double b = 0.75;

//             double rate = b * (1 + iter) / global_iteration;

//             score = (b - rate) * score + (1 - b + rate) * (old_mvcost - new_mvcost) / old_mvcost;
//         }

//         return score;
//     }
//     double update_budget_to_clients()
//     {
//         int machine_num = numprocs - 1;
//         double client_budget = Budget / machine_num;
//         return client_budget;
//     }
//     double update_mvcost_to_clients()
//     {
//         int machine_num = numprocs - 1;
//         return graph.movecost / machine_num;
//     }
//     // void update_client_thread_num_to_clients(vector<CLIENT> &c, int client_thread)
//     // {
//     //     for (auto &i : c)
//     //         i.client_thread_num = client_thread;
//     // }

//     void train(vector<CLIENT> &client)
//     {
//         int num_client = client.size();
//         // random_distribute_agent_to_machine(client);

//         log_file << "Global iteration num: " << global_iteration << endl;
//         log_file << "Local iteration num: " << client[0].local_iteration << endl;

//         for (auto &m : client)
//             m.init();
//         count_budget();

//         vector<int> client_order(num_client);
//         for (int i = 0; i < num_client; i++)
//             client_order[i] = i;
//         // 打乱顺序
//         unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

//         // omp_set_nested(1);
//         // int client_thread_num = omp_get_max_threads() / num_client;
//         // cout << "assign " << client_thread_num << " theads to each " << num_client << " clients." << endl;
//         // update_client_thread_num_to_clients(client, client_thread_num);
//         // update_budget_to_clients(client);

//         double client_train_time = 0;
//         double server_update_time = 0;
//         double client_update_time = 0;
//         auto training_begin = chrono::steady_clock::now();

//         log_file << "time\tcost\tmovecost" << endl;
//         log_file << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << graph.movecost << endl;

//         for (int it = 0; it < global_iteration; it++)
//         {
//             auto iteration_begin = chrono::steady_clock::now();

//             // update_mvcost_to_clients(client);

//             auto client_train_begin = chrono::steady_clock::now();
// #pragma omp parallel num_threads(num_client)
//             {
//                 client[omp_get_thread_num()].train(it);
//                 // client[0].train();
//                 // printf("client %d finish training\n" , omp_get_thread_num());
//             }

//             auto client_train_end = chrono::steady_clock::now();
//             client_train_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_train_end - client_train_begin).count() / 1000;

//             auto server_train_begin = chrono::steady_clock::now();
//             std::shuffle(client_order.begin(), client_order.end(), std::default_random_engine(seed));
//             random_shuffle(client_order.begin(), client_order.end());

//             vector<bool> accept(num_client, false);
//             for (int m = 0; m < num_client; m++)
//             {
//                 double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                 for (auto &x : client[client_order[m]].local_change)
//                     graph.moveVertex(x.first, x.second.choice_dc);
//                 double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;
//                 if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, it) > 0)
//                 {
//                     accept[client_order[m]] = true;
//                     printf("Accept client[%d]'s action.\n", client_order[m]);
//                 }
//                 else
//                 {
//                     // cout << Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, it) << endl;
//                     printf("Reject client[%d]'s action.\n", client_order[m]);
//                     for (auto &x : client[client_order[m]].local_change)
//                         graph.moveVertex(x.first, x.second.origin_dc);
//                 }
//             }
//             auto server_train_end = chrono::steady_clock::now();
//             server_update_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(server_train_end - server_train_begin).count() / 1000;

//             auto client_update_begin = chrono::steady_clock::now();
// #pragma omp parallel num_threads(num_client)
//             {
//                 int k = omp_get_thread_num();
//                 for (int m = 0; m < num_client; m++)
//                 {
//                     if (accept[m])
//                     {
//                         for (auto &i : client[m].local_change)
//                             client[k].graph.moveVertex(i.first, i.second.choice_dc);
//                     }
//                     else
//                     {
//                         for (auto &i : client[m].local_change)
//                             client[k].graph.moveVertex(i.first, i.second.origin_dc);
//                     }
//                 }
//             }
//             auto client_update_end = chrono::steady_clock::now();
//             client_update_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_update_end - client_update_begin).count() / 1000;

//             auto iteration_end = chrono::steady_clock::now();

//             log_file << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << graph.movecost << endl;

//             printf("\n\n[LearningAutomaton] iteration : %d / %d\n", it + 1, global_iteration);
//             printf("[LearningAutomaton] time : %f (%f)\n", graph.transfer_time, graph.transfer_time / origin_time);
//             printf("[LearningAutomaton] cost : %f (%f)\n", graph.transfer_cost, graph.transfer_cost / origin_cost);
//             printf("[LearningAutomaton] totalcost / budget : %f / %f\n", graph.movecost + graph.transfer_cost, Budget);
//             printf("[LearningAutomaton] iteration use : %f s\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(iteration_end - iteration_begin).count() / 1000);
//             graph.print(false);
//         }

//         auto training_end = chrono::steady_clock::now();
//         graph.print(false);
//         printf("[LeanrningAutomaton] client training use : %f s\n", client_train_time);
//         printf("[LeanrningAutomaton] server update use : %f s\n", server_update_time);
//         printf("[LeanrningAutomaton] client update use : %f s\n", client_update_time);

//         printf("[LeanrningAutomaton] total training use : %f s\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000);

//         log_file << "client training use : " << client_train_time << endl;
//         log_file << "server update use : " << server_update_time << endl;
//         log_file << "client update use : " << client_update_time << endl;
//         log_file << "total training use : " << (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000 << endl;
//     }
//     void train(int numprocs)
//     {

//         // wait for clients
//         printf("[%s] Server --> waiting...\n", get_time().c_str());
//         MPI_Barrier(MPI_COMM_WORLD);
//         printf("[%s] Server --> Distributing agents.\n", get_time().c_str());
//         // numprocs = 2;
//         printf("[%s] numprocs = %d\n", get_time().c_str(), numprocs);
//         vector<vector<id_type>> client_agents = random_distribute_agent_to_machine();
//         printf("[%s] Server --> Distributing agents finished.\n", get_time().c_str());
//         printf("[%s] Server --> Sending agents to clients.\n", get_time().c_str());
//         // #pragma omp parallel num_threads(numprocs - 1)
//         {
//             // int c = omp_get_thread_num() + 1;
//             for (int c = 1; c < numprocs; c++)
//             {
//                 id_type num = client_agents[c - 1].size();
//                 MPI_Send(&num, 1, MPI_UINT32_T, c, SERVER_SEND_AGENTS_NUM2CLIENTS, MPI_COMM_WORLD);
//                 MPI_Send(client_agents[c - 1].data(), (int)num, MPI_UINT32_T, c, SERVER_DISTRIBUTE_AGENT2CLIENTS, MPI_COMM_WORLD);
//                 // MPI_Send(&c, 1, MPI_INT, c, SERVER_DISTRIBUTE_AGENT2CLIENTS, MPI_COMM_WORLD);
//             }
//         }
//         printf("[%s] Server --> Sending agents finished.\n", get_time().c_str());

//         printf("[%s] client num : %d \n", get_time().c_str(), numprocs - 1);
//         printf("[%s] global iteration num : %d\n", get_time().c_str(), global_iteration);
//         printf("[%s] local iteration num : %d\n", get_time().c_str(), local_iteration);

//         log_file << "global iteration num : " << global_iteration << endl;
//         log_file << "local iteration num : " << local_iteration << endl;
//         log_file << "budget rate : " << Budget_rate << endl;
//         if(auto_sampling_rate)
//             log_file << "overhead limited : " << overhead_limit << endl;


//         log_file << "time\tcost\tmovecost\tregret rate\tregret num\tchange num" << endl;
//         log_file << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << graph.movecost << endl;

//         count_budget();
//         double local_budget = update_budget_to_clients();
//         MPI_Bcast(&local_budget, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

//         MPI_Barrier(MPI_COMM_WORLD);

//         vector<int> client_local_change_size(numprocs);
//         vector<int> disp(numprocs);
//         vector<uint> client_change;
//         vector<uint> apply_change;
//         // id_type client_local_change_size[numprocs];
//         id_type local_change_size = 0;

//         double client_train_time = 0;
//         double server_update_time = 0;
//         double client_update_time = 0;

//         // vector<int> client_order(numprocs - 1);
//         // iota(begin(client_order), end(client_order), 1);

//         auto training_begin = chrono::steady_clock::now();

//         for (int global = 0; global < global_iteration; global++)
//         {
//             auto client_train_begin = chrono::steady_clock::now();
//             auto iteration_begin = chrono::steady_clock::now();
//             if (auto_sampling_rate)
//                 printf("[%s] Sampling rate : %.4f\n", get_time().c_str(), sampling_rate);

//             MPI_Gather(&local_change_size, 1, MPI_UINT32_T, client_local_change_size.data(), 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
//             for (int i = 0; i < client_local_change_size.size(); i++)
//                 printf("client %d --> change size = %u \n", i, client_local_change_size[i]);
//             client_change.clear();
//             uint change_sum = accumulate(client_local_change_size.begin(), client_local_change_size.end(), 0);
//             client_change.resize(change_sum);
//             apply_change.resize(change_sum);
//             for (int i = 2; i < numprocs; i++)
//                 disp[i] = disp[i - 1] + client_local_change_size[i - 1];
//             MPI_Gatherv(NULL, 0, MPI_UINT32_T, client_change.data(), client_local_change_size.data(), disp.data(), MPI_UINT32_T, 0, MPI_COMM_WORLD);

//             auto client_train_end = chrono::steady_clock::now();
//             client_train_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_train_end - client_train_begin).count() / 1000;
//             auto server_train_begin = chrono::steady_clock::now();

//             uint regret = 0;

//             for (uint j = 0; j < change_sum; j += 2)
//             {
//                 id_type i = (j + disp[(global + 1) % (numprocs - 1)]) % change_sum;
//                 id_type v = client_change[i];
//                 int choice_dc = client_change[i + 1];
//                 apply_change[i] = v;
//                 apply_change[i + 1] = graph.vertex[v].current_dc;
//                 double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                 graph.moveVertex(v, choice_dc);
//                 double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;
//                 if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, global) > 0)
//                 {
//                     apply_change[i + 1] = choice_dc;
//                 }
//                 else
//                 {
//                     apply_change[i + 1] = choice_dc;
//                     // graph.moveVertex(v, apply_change[i + 1]);
//                     regret += 1;
//                 }
//             }
//             printf("[LearningAutomaton] regret rate %f(%u / %u)\n", 2. * regret / change_sum, regret, change_sum >> 1);
//             auto server_train_end = chrono::steady_clock::now();
//             server_update_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(server_train_end - server_train_begin).count() / 1000;
//             if (global < global_iteration - 1)
//             {
//                 auto client_update_begin = chrono::steady_clock::now();
//                 MPI_Bcast(&change_sum, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
//                 // MPI_Barrier(MPI_COMM_WORLD);

//                 // for(int i = 0; i < 10; i++)
//                 //         printf("test-client: %u -> %d\n", apply_change[i*2], apply_change[i*2+1]);
//                 MPI_Bcast(apply_change.data(), change_sum, MPI_UINT32_T, 0, MPI_COMM_WORLD);

//                 double client_mvcost = update_mvcost_to_clients();
//                 MPI_Bcast(&client_mvcost, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
//                 printf("[%s] Server --> waiting...\n", get_time().c_str());
//                 MPI_Barrier(MPI_COMM_WORLD);
//                 auto client_update_end = chrono::steady_clock::now();
//                 client_update_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_update_end - client_update_begin).count() / 1000;
//             }

//             log_file << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << graph.movecost << '\t' << 2. * regret / change_sum << '\t' << regret << '\t' << (change_sum >> 1) << endl;

//             auto iteration_end = chrono::steady_clock::now();
//             printf("[LearningAutomaton] iteration : %d / %d\n", global + 1, global_iteration);
//             printf("[LearningAutomaton] time : %f (%f)\n", graph.transfer_time, graph.transfer_time / origin_time);
//             printf("[LearningAutomaton] cost : %f (%f)\n", graph.transfer_cost, graph.transfer_cost / origin_cost);
//             printf("[LearningAutomaton] totalcost / budget : %f / %f\n", graph.movecost + graph.transfer_cost, Budget);
//             printf("[LearningAutomaton] iteration use : %f s\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(iteration_end - iteration_begin).count() / 1000);
//             graph.print(false);
//             printf("\n\n");
//         }
//         auto training_end = chrono::steady_clock::now();
//         graph.print(false);
//         printf("[LeanrningAutomaton] client training use : %f s\n", client_train_time);
//         printf("[LeanrningAutomaton] server update use : %f s\n", server_update_time);
//         printf("[LeanrningAutomaton] client update use : %f s\n", client_update_time);

//         printf("[LeanrningAutomaton] total training use : %f s\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000);

//         log_file << "client training use :\t" << client_train_time << endl;
//         log_file << "server update use :\t" << server_update_time << endl;
//         log_file << "client update use :\t" << client_update_time << endl;
//         log_file << "total training use :\t" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000 << endl;
//     }
// };

// class Client_without_Server : public MACHINE
// {
// public:
//     double local_budget;
//     int local_iteration = 1; // 迭代次数
//     int global_iteration = 10;
//     double Budget;
//     double origin_time; // 初始通信时间
//     double origin_cost; // 初始开销
//     double global_movecost = 0;
//     ofstream log_file;
//     double sampling_rate;

//     // int my_id;

//     vector<int> pre_choice;

//     // vector<agent_move> local_change;
//     unordered_map<id_type, agent_move> local_change;
//     vector<uint> local_change_v;

//     vector<vector<double>> probability;

//     // test
//     vector<id_type> agent_order;

//     Client_without_Server(string graph_file_name, string network_file_name, int global_it, int local_it)
//         : MACHINE(graph_file_name, network_file_name, myid == 0 ? true : false)
//     {
//         // myid = omp_get_thread_num();
//         // sampling_rate = default_sampling_rate;
//         sampling_rate = 1. / global_iteration;
//         global_iteration = global_it;
//         local_iteration = local_it;
//         printf("[%s] Client %d reads graph finished.\n", get_time().c_str(), myid);
//         MPI_Barrier(MPI_COMM_WORLD);
//         if (myid == 0) // "server"
//         {
//             graph.display_graph_state = true;
//             graph.print();

//             log_file.open("LOG.txt", ios::out | ios::trunc);
//             if (!log_file.is_open())
//             {
//                 cout << "can't open LOG file" << endl;
//                 exit(-1);
//             }
//             log_file.setf(ios::fixed);
//             log_file.precision(5);

//             log_file << "Graph file name: " << graph.GRAPH_FILE_NAME << endl;
//             log_file << "Network file name: " << graph.NETWORK_FILE_NAME << endl;
//             log_file << "Client num: " << numprocs << endl;
//             log_file << "Serverless mode" << endl; 
//             log_file << "global iteration num : " << global_iteration << endl;
//             log_file << "local iteration num : " << local_iteration << endl;
//             log_file << "budget rate : " << Budget_rate << endl;
//         if(auto_sampling_rate)
//             log_file << "overhead limited : " << overhead_limit << endl;

//             printf("[%s] client num : %d \n", get_time().c_str(), numprocs - 1);
//             printf("[%s] global iteration num : %d\n", get_time().c_str(), global_iteration);
//             printf("[%s] local iteration num : %d\n", get_time().c_str(), local_iteration);
//         }
//     }
//     ~Client_without_Server()
//     {
//         log_file.close();
//     }
//     vector<double> moveVirtualVertex(id_type v, int it)
//     // 以当前环境为参考，移动顶点到所有dc，返回每个dc的score情况
//     {
//         int &DC_num = graph.DC_num;
//         vector<Vertex> &vertex = graph.vertex;

//         vector<std::vector<Mirror>> &mirror = graph.mirror;

//         vector<double> score(DC_num);

//         // unordered_map<id_type, int>

//         for (int dc = 0; dc < DC_num; dc++)
//         {
//             if (vertex[v].current_dc == dc)
//                 continue;
//             double movecost = graph.movecost;
//             vector<DataCenter> DC = graph.DC;

//             int origin_dc = vertex[v].current_dc;
//             int init_dc = vertex[v].init_dc;

//             if (origin_dc == init_dc)
//             {
//                 movecost += MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
//             }
//             else if (dc == init_dc)
//             {
//                 movecost -= MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
//             }
//             if (vertex[v].is_high_degree)
//             {
//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (mirror[i][v].in_use && mirror[i][v].local_in_degree > 0)
//                         DC[origin_dc].gather_download_wan -= DATA_UNIT;
//                     if (mirror[i][v].in_use && mirror[i][v].local_out_degree > 0)
//                         DC[origin_dc].apply_upload_wan -= DATA_UNIT;
//                 }
//                 int mirrorin = vertex[v].local_in_degree;
//                 int mirrorout = vertex[v].local_out_degree;

//                 if (mirrorin > 0)
//                     DC[origin_dc].gather_upload_wan += DATA_UNIT;
//                 if (mirrorout > 0)
//                     DC[origin_dc].apply_download_wan += DATA_UNIT;

//                 if (mirror[dc][v].local_in_degree > 0)
//                     DC[dc].gather_upload_wan -= DATA_UNIT;
//                 if (mirror[dc][v].local_out_degree > 0)
//                     DC[dc].apply_download_wan -= DATA_UNIT;

//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (i != dc && i != origin_dc && mirror[i][v].local_in_degree > 0)
//                         DC[dc].gather_download_wan += DATA_UNIT;
//                     if (i != dc && i != origin_dc && mirror[i][v].local_out_degree > 0)
//                         DC[dc].apply_upload_wan += DATA_UNIT;
//                 }

//                 if (mirrorin > 0)
//                     DC[dc].gather_download_wan += DATA_UNIT;
//                 if (mirrorout > 0)
//                     DC[dc].apply_upload_wan += DATA_UNIT;

//                 for (auto &out_neighbour : vertex[v].out_edge)
//                 {
//                     if (vertex[out_neighbour].is_high_degree)
//                     {
//                         mirrorout--;
//                         if (mirrorout == 0)
//                         {
//                             DC[origin_dc].apply_download_wan -= DATA_UNIT;
//                             DC[dc].apply_upload_wan -= DATA_UNIT;
//                         }

//                         if (vertex[out_neighbour].current_dc == origin_dc)
//                         {
//                             // vertex[out_neighbour].local_in_degree--;

//                             if (mirror[dc][out_neighbour].local_in_degree == 0)
//                             {
//                                 DC[dc].gather_upload_wan += DATA_UNIT;
//                                 DC[origin_dc].gather_download_wan += DATA_UNIT;
//                             }
//                             // mirror[dc][out_neighbour].add(1, 0);
//                         }
//                         else
//                         {
//                             int out_neighbour_dc = vertex[out_neighbour].current_dc;
//                             // mirror[origin_dc][out_neighbour].local_in_degree--;
//                             if (mirror[origin_dc][out_neighbour].local_in_degree == 1)
//                             {
//                                 DC[origin_dc].gather_upload_wan -= DATA_UNIT;
//                                 DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
//                             }
//                             if (out_neighbour_dc != dc)

//                             {
//                                 if (mirror[dc][out_neighbour].local_in_degree == 0)
//                                 {
//                                     DC[dc].gather_upload_wan += DATA_UNIT;
//                                     DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
//                                 }
//                                 // mirror[dc][out_neighbour].add(1, 0);
//                             }
//                         }
//                     }
//                 }
//             }
//             else
//             {
//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (mirror[i][v].in_use && mirror[i][v].local_out_degree > 0)
//                         DC[origin_dc].apply_upload_wan -= DATA_UNIT;
//                 }

//                 if (vertex[v].local_in_degree > 0)
//                     DC[origin_dc].gather_upload_wan += DATA_UNIT,
//                         DC[dc].gather_download_wan += DATA_UNIT;
//                 if (vertex[v].local_out_degree > 0)
//                     DC[origin_dc].apply_download_wan += DATA_UNIT,
//                         DC[dc].apply_upload_wan += DATA_UNIT;

//                 if (mirror[dc][v].in_use)
//                 {
//                     // if (mirror[dc][v].local_in_degree > 0)
//                     // DC[dc].gather_upload_wan -= DATA_UNIT;
//                     if (mirror[dc][v].local_out_degree > 0)
//                         DC[dc].apply_download_wan -= DATA_UNIT;
//                 }

//                 int mirrorin = vertex[v].local_in_degree;
//                 int mirrorout = vertex[v].local_out_degree;
//                 // mirror[dc][v].in_use = false;
//                 // mirror[dc][v].del();

//                 for (int i = 0; i < DC_num; i++)
//                 {
//                     if (i != dc && i != origin_dc && mirror[i][v].local_out_degree > 0)
//                         DC[dc].apply_upload_wan += DATA_UNIT;
//                 }

//                 for (auto &in_neighbour : vertex[v].in_edge)
//                 {
//                     // mirror[origin_dc][v].local_in_degree--;
//                     // vertex[v].local_in_degree++;
//                     mirrorin--;
//                     if (mirrorin == 0)
//                     {
//                         DC[origin_dc].gather_upload_wan -= DATA_UNIT;
//                         DC[dc].gather_download_wan -= DATA_UNIT;
//                     }
//                     // if (vertex[in_neighbour].is_high_degree)
//                     {
//                         int in_neighbour_dc = vertex[in_neighbour].current_dc;
//                         if (in_neighbour_dc == origin_dc)
//                         {
//                             // vertex[in_neighbour].local_out_degree--;
//                             if (mirror[dc][in_neighbour].local_out_degree == 0)
//                             {
//                                 DC[dc].apply_download_wan += DATA_UNIT;
//                                 DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
//                             }
//                             // mirror[dc][in_neighbour].add(0, 1);
//                         }
//                         else
//                         {
//                             // mirror[origin_dc][in_neighbour].local_out_degree--;
//                             if (mirror[origin_dc][in_neighbour].local_out_degree == 1)
//                             {
//                                 DC[origin_dc].apply_download_wan -= DATA_UNIT;
//                                 DC[in_neighbour_dc].apply_upload_wan -= DATA_UNIT;
//                             }
//                             if (in_neighbour_dc != dc)
//                             {
//                                 if (mirror[dc][in_neighbour].local_out_degree == 0)
//                                 {
//                                     DC[dc].apply_download_wan += DATA_UNIT;
//                                     DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
//                                 }
//                                 // mirror[dc][in_neighbour].add(0, 1);
//                             }
//                         }
//                     }
//                 }
//                 for (auto &out_neighbour : vertex[v].out_edge)
//                 {
//                     if (vertex[out_neighbour].is_high_degree)
//                     {
//                         mirrorout--;
//                         if (mirrorout == 0)
//                         {
//                             DC[origin_dc].apply_download_wan -= DATA_UNIT;
//                             DC[dc].apply_upload_wan -= DATA_UNIT;
//                         }

//                         if (vertex[out_neighbour].current_dc == origin_dc)
//                         {
//                             // vertex[out_neighbour].local_in_degree--;

//                             if (mirror[dc][out_neighbour].local_in_degree == 0)
//                             {
//                                 DC[dc].gather_upload_wan += DATA_UNIT;
//                                 DC[origin_dc].gather_download_wan += DATA_UNIT;
//                             }
//                             // mirror[dc][out_neighbour].add(1, 0);
//                         }
//                         else
//                         {
//                             int out_neighbour_dc = vertex[out_neighbour].current_dc;
//                             // mirror[origin_dc][out_neighbour].local_in_degree--;
//                             if (mirror[origin_dc][out_neighbour].local_in_degree == 1)
//                             {
//                                 DC[origin_dc].gather_upload_wan -= DATA_UNIT;
//                                 DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
//                             }
//                             if (out_neighbour_dc != dc)

//                             {
//                                 if (mirror[dc][out_neighbour].local_in_degree == 0)
//                                 {
//                                     DC[dc].gather_upload_wan += DATA_UNIT;
//                                     DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
//                                 }
//                                 // mirror[dc][out_neighbour].add(1, 0);
//                             }
//                         }
//                     }
//                 }
//             }
//             double t, p;

//             graph.calculate_network_time_price(t, p, DC);

//             score[dc] = Score(graph.transfer_time, graph.transfer_cost, graph.movecost, t, p, movecost, it);
//         }

//         return score;
//     }
//     double Score(double old_time, double old_cost, double old_mvcost, double new_time, double new_cost, double new_mvcost, int iter)
//     // 计算得分
//     {
//         double score;
//         // if (new_cost + new_mvcost < Budget)
//         score = (old_time - new_time) / old_time + (old_cost - new_cost) / old_cost;
//         // score = (old_time - new_time) / old_time;
//         // else

//         if (new_mvcost + new_cost > local_budget)
//         {
//             double old_ = old_cost + old_mvcost;
//             double new_ = new_cost + new_mvcost;
//             double b = 0.75;

//             double rate = b * (1 + iter) / local_iteration;

//             score = (b - rate) * score + (1 - b + rate) * (old_mvcost - new_mvcost) / old_mvcost;
//         }

//         return score;
//     }

//     vector<vector<id_type>> random_distribute_agent_to_machine()
//     {
//         int machine_num = numprocs;
//         vector<int> which_machine(graph.vertex_num);
// #pragma omp parallel for
//         for (id_type i = 0; i < graph.vertex_num; i++)
//             which_machine[i] = rand() % machine_num;

//         vector<vector<id_type>> client_agents(machine_num);
// #pragma omp parallel for num_threads(machine_num)
//         for (int i = 0; i < machine_num; i++)
//             for (id_type j = 0; j < graph.vertex_num; j++)
//                 if (which_machine[j] == i)
//                     client_agents[i].push_back(j);

//         // cout << "Finished agent distribution:" << endl;

//         cout << "Using Random distribute agent." << endl;
//         log_file << "Using Random distribute agent." << endl;

//         for (int i = 0; i < machine_num; i++)
//         {
//             // reverse(client_agents[i].begin(), client_agents[i].end());
//             printf("Machine[%d] : %lu\tangents.\n", i, client_agents[i].size());
//         }
//         return client_agents;
//     }
//     vector<vector<id_type>> mod_distribute_agent_to_machine()
//     {
//         int machine_num = numprocs;
//         vector<int> which_machine(graph.vertex_num);
// #pragma omp parallel for
//         for (id_type i = 0; i < graph.vertex_num; i++)
//             which_machine[i] = i % machine_num;

//         vector<vector<id_type>> client_agents(machine_num);
// #pragma omp parallel for num_threads(machine_num)
//         for (int i = 0; i < machine_num; i++)
//             for (id_type j = 0; j < graph.vertex_num; j++)
//                 if (which_machine[j] == i)
//                     client_agents[i].push_back(j);

//         // cout << "Finished agent distribution:" << endl;

//         cout << "Using Random distribute agent." << endl;
//         log_file << "Using Random distribute agent." << endl;

//         for (int i = 0; i < machine_num; i++)
//         {
//             // reverse(client_agents[i].begin(), client_agents[i].end());
//             printf("Machine[%d] : %lu\tangents.\n", i, client_agents[i].size());
//         }
//         return client_agents;
//     }

//     void distribute_agents_to_clients()
//     {
//         if (myid == 0)
//         {
//             printf("[%s] Server --> Distributing agents......\n", get_time().c_str());
//             printf("[%s] numprocs = %d\n", get_time().c_str(), numprocs);

//             // vector<vector<id_type>> client_agents = random_distribute_agent_to_machine();

//             vector<vector<id_type>> client_agents = mod_distribute_agent_to_machine();
//             printf("[%s] Server --> Distributing agents finished.\n", get_time().c_str());
//             printf("[%s] Server --> Sending agents to clients......\n", get_time().c_str());

//             for (int c = 1; c < numprocs; c++)
//             {
//                 id_type num = client_agents[c].size();
//                 MPI_Send(&num, 1, MPI_UINT32_T, c, SERVER_SEND_AGENTS_NUM2CLIENTS, MPI_COMM_WORLD);
//                 MPI_Send(client_agents[c].data(), (int)num, MPI_UINT32_T, c, SERVER_DISTRIBUTE_AGENT2CLIENTS, MPI_COMM_WORLD);
//                 // MPI_Send(&c, 1, MPI_INT, c, SERVER_DISTRIBUTE_AGENT2CLIENTS, MPI_COMM_WORLD);
//             }

//             printf("[%s] Server --> Sending agents finished.\n", get_time().c_str());
//             agent_num = client_agents[0].size();
//             agent_vector = client_agents[0];

//             printf("[%s] Server %d --> Received %d agents num.\n", get_time().c_str(), myid, agent_num);
//             printf("[%s] Server %d --> Received %ld agents finished.\n", get_time().c_str(), myid, agent_vector.size());
//         }
//         else
//         {
//             MPI_Recv(&agent_num, 1, MPI_UINT32_T, 0, SERVER_SEND_AGENTS_NUM2CLIENTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//             printf("[%s] Client %d --> Received %d agents num.\n", get_time().c_str(), myid, agent_num);
//             agent_vector.resize(agent_num);
//             MPI_Recv(agent_vector.data(), agent_num, MPI_UINT32_T, 0, SERVER_DISTRIBUTE_AGENT2CLIENTS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//             printf("[%s] Client %d --> Received %ld agents finished.\n", get_time().c_str(), myid, agent_vector.size());
//         }
//     }
//     void init()
//     {
//         probability = vector<vector<double>>(agent_vector.size(), vector<double>(graph.DC_num, 1. / graph.DC_num));
//         pre_choice = vector<int>(agent_vector.size(), -1);
//         sort(agent_vector.begin(), agent_vector.end(), [&](id_type a, id_type b)
//              { return graph.vertex[a].in_degree < graph.vertex[b].in_degree; });
//         // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
//         // std::shuffle(agent_vector.begin(), agent_vector.end(), std::default_random_engine(seed));
//         // random_shuffle(agent_vector.begin(), agent_vector.end());
//         // omp_set_nested(1);
//         // sort(agent_vector.begin(), agent_vector.end(), [&](id_type a, id_type b)
//         //  { return graph.vertex[a].num_mirror > graph.vertex[b].num_mirror; });
//         agent_order.resize(agent_vector.size());
//         iota(begin(agent_order), end(agent_order), 0);

//         unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));
//     }
//     void count_budget()
//     {
//         int max_price_dc = 0;                       // 记录最贵的服务器
//         double max_price = graph.DC[0].UploadPrice; // 记录最贵的服务器上传价格
//         for (int i = 0; i < graph.DC_num; i++)
//         {
//             if (max_price < graph.DC[i].UploadPrice)
//                 max_price_dc = i, max_price = graph.DC[i].UploadPrice;
//             Budget += graph.DC[i].vertex_num * graph.DC[i].UploadPrice;
//         }
//         Budget -= graph.DC[max_price_dc].vertex_num * graph.DC[max_price_dc].UploadPrice;
//         Budget *= Budget_rate;
//         Budget *= MOVE_DATA_UNIT; // Budget为将所有顶点迁移到最贵的DC所消耗的价格的budget_rate
//         origin_time = graph.transfer_time;
//         origin_cost = graph.transfer_cost;

//         local_budget = Budget / numprocs;

//         if (myid == 0)
//         {
//             printf("[%s] Budget = %f, local budget = %f\n", get_time().c_str(), Budget, local_budget);
//         }
//     }

//     inline int make_decision_greedy(id_type &agent_index)
//     {
//         int choice_dc = max_element(probability[agent_index].begin(), probability[agent_index].end()) - probability[agent_index].begin();
//         return choice_dc;
//     }

//     void batch_sampling_base_on_global_iteration(int global_it, int local_it)
//     {
//         const int agents_per_it = agent_vector.size() / global_iteration;
//         // printf("%d~%d\n", global_it * agents_per_it, (global_it + 1) * agents_per_it);

// #pragma omp parallel for
//         // for (id_type vv = 0; vv < client_thread_num; vv++)
//         for (id_type vv = global_it * agents_per_it; vv < (global_it + 1) * agents_per_it; vv++)
//         {
//             id_type v = agent_vector[vv];
//             vector<double> s = moveVirtualVertex(v, local_it); // local iteration 默认为1了
//             int max_s = max_element(s.begin(), s.end()) - s.begin();
//             for (int dc = 0; dc < graph.DC_num; dc++)
//             {
//                 if (max_s == dc)
//                     probability[vv][dc] += alpha * (1 - probability[vv][dc]);
//                 else
//                     probability[vv][dc] = (1 - alpha) * probability[vv][dc];
//             }
//             pre_choice[vv] = make_decision_greedy(vv);
//         }
//     }

//     void random_sampling_agents(int local_it)
//     {
//         // const double random_sampling_rate = 1. / global_iteration;
//         // #pragma omp parallel for
//         //         // for (id_type vv = 0; vv < client_thread_num; vv++)
//         //         for (id_type vv = 0; vv < agent_vector.size(); vv++)
//         //         {

//         //             const int sp_rate = sampling_rate * RAND_MAX;
//         //             if (rand() < sp_rate)
//         //             {
//         //                 id_type v = agent_vector[vv];
//         //                 vector<double> s = moveVirtualVertex(v, local_it);
//         //                 int max_s = max_element(s.begin(), s.end()) - s.begin();
//         //                 for (int dc = 0; dc < graph.DC_num; dc++)
//         //                 {
//         //                     if (max_s == dc)
//         //                         probability[vv][dc] += alpha * (1 - probability[vv][dc]);
//         //                     else
//         //                         probability[vv][dc] = (1 - alpha) * probability[vv][dc];
//         //                 }
//         //                 pre_choice[vv] = make_decision_greedy(vv);
//         //             }
//         //             else
//         //                 pre_choice[vv] = -1;
//         //         }

//         int sampling_agents_num = sampling_rate * agent_vector.size();
//         // printf("%d~%d\n", global_it * agents_per_it, (global_it + 1) * agents_per_it);

// #pragma omp parallel for
//         // for (id_type vv = 0; vv < client_thread_num; vv++)
//         for (id_type vvv = 0; vvv < sampling_agents_num; vvv++)
//         {
//             id_type vv = agent_order[vvv];
//             id_type v = agent_vector[vv];
//             vector<double> s = moveVirtualVertex(v, local_it); // local iteration 默认为1了
//             int max_s = max_element(s.begin(), s.end()) - s.begin();
//             for (int dc = 0; dc < graph.DC_num; dc++)
//             {
//                 if (max_s == dc)
//                     probability[vv][dc] += alpha * (1 - probability[vv][dc]);
//                 else
//                     probability[vv][dc] = (1 - alpha) * probability[vv][dc];
//             }
//             pre_choice[vv] = make_decision_greedy(vv);
//         }
//     }

//     void train_agents_batching(int global_it)
//     {
//         local_change.clear();
//         local_change_v.clear();
//         // assert(local_iteration == 1);

//         for (int local_it = 0; local_it < local_iteration; local_it++)
//         {
//             batch_sampling_base_on_global_iteration(global_it, local_it);

//             const int agents_per_it = agent_vector.size() / global_iteration;
//             for (id_type i = global_it * agents_per_it; i < (global_it + 1) * agents_per_it; i++)
//             {
//                 id_type v = agent_vector[i];
//                 int origin_dc = graph.vertex[v].current_dc;
//                 double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                 int choice_dc = pre_choice[i];

//                 graph.moveVertex(v, choice_dc);
//                 double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;

//                 if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, local_it) > 0)
//                 {
//                     for (int dc = 0; dc < graph.DC_num; dc++)
//                     {
//                         if (choice_dc == dc)
//                             probability[i][dc] += alpha * (1 - probability[i][dc]);
//                         else
//                             probability[i][dc] = (1 - alpha) * probability[i][dc];
//                     }
//                     if (local_change.count(v))
//                     {
//                         if (choice_dc == local_change[v].origin_dc)
//                             local_change.erase(v);
//                         else
//                             local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     }
//                     else
//                         local_change[v] = {v, origin_dc, choice_dc};
//                     // if (local_change.count(v))
//                     //     local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     // else
//                     //     local_change[v] = {v, origin_dc, choice_dc};
//                 }
//                 else
//                 {
//                     graph.moveVertex(v, origin_dc);
//                 }
//             }
//         }
//         local_change_v.reserve(local_change.size() * 2);
//         for (auto &x : local_change)
//             local_change_v.emplace_back(x.first), local_change_v.emplace_back(x.second.choice_dc);
//     }

//     void train_agents_random(int global_it)
//     {
//         local_change.clear();
//         local_change_v.clear();
//         // assert(local_iteration == 1);

//         unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
//         std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));

//         for (int local_it = 0; local_it < local_iteration; local_it++)
//         {
//             auto shuffle_begin = chrono::steady_clock::now();
//             random_sampling_agents(local_it);
//             auto shuffle_end = chrono::steady_clock::now();
//             cout << "------" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(shuffle_end - shuffle_begin).count() / 1000 << endl;

//             int sampling_agents_num = sampling_rate * agent_vector.size();
//             for (id_type ii = 0; ii < sampling_agents_num; ii++)
//             // for(id_type i = 0; i < agent_vector.size(); i++)
//             {

//                 id_type i = agent_order[ii];
//                 id_type v = agent_vector[i];
//                 int origin_dc = graph.vertex[v].current_dc;
//                 double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                 int choice_dc = pre_choice[i];
//                 // if(choice_dc == -1) continue;

//                 graph.moveVertex(v, choice_dc);
//                 double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;

//                 if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, local_it) > 0)
//                 {
//                     for (int dc = 0; dc < graph.DC_num; dc++)
//                     {
//                         if (choice_dc == dc)
//                             probability[i][dc] += alpha * (1 - probability[i][dc]);
//                         else
//                             probability[i][dc] = (1 - alpha) * probability[i][dc];
//                     }
//                     if (local_change.count(v))
//                     {
//                         if (choice_dc == local_change[v].origin_dc)
//                             local_change.erase(v);
//                         else
//                             local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     }
//                     else
//                         local_change[v] = {v, origin_dc, choice_dc};
//                     // if (local_change.count(v))
//                     //     local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                     // else
//                     //     local_change[v] = {v, origin_dc, choice_dc};
//                 }
//                 else
//                 {
//                     graph.moveVertex(v, origin_dc);
//                 }
//             }
//         }
//         local_change_v.reserve(local_change.size() * 2);
//         for (auto &x : local_change)
//             local_change_v.emplace_back(x.first), local_change_v.emplace_back(x.second.choice_dc);
//     }
//     void update_sampling_rate(int cur_it, vector<double> &x, double left_overhead)
//     {
//         if (!auto_sampling_rate)
//             return;
//         int max_len = 10;
//         int min_len = min(cur_it + 1, max_len);
//         double sum = 0;
//         for (int i = cur_it + 1 - min_len; i < cur_it + 1; i++)
//             sum += x[i];
//         double left_time_per_iteration = left_overhead / (global_iteration - cur_it - 1);
//         sampling_rate = max(0., sum / min_len * left_time_per_iteration);
//         sampling_rate = min(1., sampling_rate);
//     }

//     void train_agents_batching_with_random_sampling(int global_it)
//     {
//         local_change.clear();
//         local_change_v.clear();
//         // assert(local_iteration == 1);

        

//         if (global_it < switch_iteration)
//         {
//             sampling_rate = 1. / global_iteration;
//             // printf("using random sampling : %f\n", sampling_rate);
//             unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
//             std::shuffle(agent_order.begin(), agent_order.end(), std::default_random_engine(seed));

//             for (int local_it = 0; local_it < local_iteration; local_it++)
//             {
//                 random_sampling_agents(local_it);
                
//                 int sampling_agents_num = sampling_rate * agent_vector.size();
//                 for (id_type ii = 0; ii < sampling_agents_num; ii++)
//                 // for(id_type i = 0; i < agent_vector.size(); i++)
//                 {

//                     id_type i = agent_order[ii];
//                     id_type v = agent_vector[i];
//                     int origin_dc = graph.vertex[v].current_dc;
//                     double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                     int choice_dc = pre_choice[i];
//                     // if(choice_dc == -1) continue;

//                     graph.moveVertex(v, choice_dc);
//                     double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;

//                     if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, local_it) > 0)
//                     {
//                         for (int dc = 0; dc < graph.DC_num; dc++)
//                         {
//                             if (choice_dc == dc)
//                                 probability[i][dc] += alpha * (1 - probability[i][dc]);
//                             else
//                                 probability[i][dc] = (1 - alpha) * probability[i][dc];
//                         }
//                         if (local_change.count(v))
//                         {
//                             if (choice_dc == local_change[v].origin_dc)
//                                 local_change.erase(v);
//                             else
//                                 local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                         }
//                         else
//                             local_change[v] = {v, origin_dc, choice_dc};
//                         // if (local_change.count(v))
//                         //     local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                         // else
//                         //     local_change[v] = {v, origin_dc, choice_dc};
//                     }
//                     else
//                     {
//                         graph.moveVertex(v, origin_dc);
//                     }
//                 }
//             }
//         }
//         else
//         {
            
//             // printf("using kdegree\n");
//             for (int local_it = 0; local_it < local_iteration; local_it++)
//             {
//                 batch_sampling_base_on_global_iteration(global_it - switch_iteration, local_it);

//                 const int agents_per_it = agent_vector.size() / global_iteration;
//                 for (id_type i = (global_it - switch_iteration) * agents_per_it; i < (global_it - switch_iteration + 1) * agents_per_it; i++)
//                 {
//                     id_type v = agent_vector[i];
//                     int origin_dc = graph.vertex[v].current_dc;
//                     double old_time = graph.transfer_time, old_cost = graph.transfer_cost, old_mvcost = graph.movecost;
//                     int choice_dc = pre_choice[i];

//                     graph.moveVertex(v, choice_dc);
//                     double new_time = graph.transfer_time, new_cost = graph.transfer_cost, new_mvcost = graph.movecost;

//                     if (Score(old_time, old_cost, old_mvcost, new_time, new_cost, new_mvcost, local_it) > 0)
//                     {
//                         for (int dc = 0; dc < graph.DC_num; dc++)
//                         {
//                             if (choice_dc == dc)
//                                 probability[i][dc] += alpha * (1 - probability[i][dc]);
//                             else
//                                 probability[i][dc] = (1 - alpha) * probability[i][dc];
//                         }
//                         if (local_change.count(v))
//                         {
//                             if (choice_dc == local_change[v].origin_dc)
//                                 local_change.erase(v);
//                             else
//                                 local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                         }
//                         else
//                             local_change[v] = {v, origin_dc, choice_dc};
//                         // if (local_change.count(v))
//                         //     local_change[v] = {v, local_change[v].origin_dc, choice_dc};
//                         // else
//                         //     local_change[v] = {v, origin_dc, choice_dc};
//                     }
//                     else
//                     {
//                         graph.moveVertex(v, origin_dc);
//                     }
//                 }
//             }
//         }
//         local_change_v.reserve(local_change.size() * 2);
//         for (auto &x : local_change)
//             local_change_v.emplace_back(x.first), local_change_v.emplace_back(x.second.choice_dc);
//     }

//     void train()
//     {
//         printf("[%s] Client %d begins training.\n", get_time().c_str(), myid);

//         double client_train_time = 0;
//         double client_update_time = 0;

//         log_file << "time\tcost\tmovecost" << endl;
//         log_file << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << graph.movecost << endl;

//         uint change_sum;
//         vector<uint> apply_change;

//         vector<int> client_local_change_size(numprocs);
//         vector<int> disp(numprocs);
//         vector<uint> client_change;

//         vector<double> sampling_rate_per_iteration(global_iteration);
//         vector<double> overhead_per_iteration(global_iteration);
//         vector<double> samplingrate_overhead_rate(global_iteration);
//         double left_overhead = overhead_limit;
//         double overhead_use = 0;

//         MPI_Barrier(MPI_COMM_WORLD);
//         auto training_begin = chrono::steady_clock::now();

//         for (int global_it = 0; global_it < global_iteration; global_it++)
//         {
//             MPI_Barrier(MPI_COMM_WORLD);

//             sampling_rate_per_iteration[global_it] = sampling_rate;

//             auto client_train_begin = chrono::steady_clock::now();
//             auto iteration_begin = chrono::steady_clock::now();

//             // train_agents_batching(global_it);
//             train_agents_random(global_it);
//             // train_agents_batching_with_random_sampling(global_it);
//             // printf("[%s] Client %d --> Training finished with %ld local changes\n", get_time().c_str(), myid, local_change.size());

//             int send_local_change_size = local_change.size() << 1;

//             MPI_Allgather(&send_local_change_size, 1, MPI_INT, client_local_change_size.data(), 1, MPI_INT, MPI_COMM_WORLD);

//             for (int i = 1; i < numprocs; i++)
//                 disp[i] = disp[i - 1] + client_local_change_size[i - 1];
//             int client_local_change_space_size = accumulate(begin(client_local_change_size), end(client_local_change_size), 0);
//             client_change.resize(client_local_change_space_size);

//             MPI_Allgatherv(local_change_v.data(), local_change_v.size(), MPI_UINT32_T, client_change.data(), client_local_change_size.data(), disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

//             auto client_train_end = chrono::steady_clock::now();
//             client_train_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_train_end - client_train_begin).count() / 1000;

//             auto client_update_begin = chrono::steady_clock::now();

//             double save_movecost = graph.movecost;
//             for (uint i = 0; i < client_local_change_space_size; i += 2)
//             {
//                 id_type v = client_change[i];
//                 int choice_dc = client_change[i + 1];
//                 graph.moveVertex(v, choice_dc);
//             }
//             double delta_move_cost = graph.movecost - save_movecost;
//             global_movecost += delta_move_cost;
//             graph.movecost = save_movecost;

//             auto client_update_end = chrono::steady_clock::now();
//             client_update_time += (double)std::chrono::duration_cast<std::chrono::milliseconds>(client_update_end - client_update_begin).count() / 1000;
//             MPI_Barrier(MPI_COMM_WORLD);
//             auto iteration_end = chrono::steady_clock::now();
//             double iteration_use_overhead = (double)std::chrono::duration_cast<std::chrono::milliseconds>(iteration_end - iteration_begin).count() / 1000;
//             overhead_per_iteration[global_it] = iteration_use_overhead;
//             overhead_use += iteration_use_overhead;
//             samplingrate_overhead_rate[global_it] = sampling_rate / iteration_use_overhead;

//             if (myid == 0)
//             {
//                 // for (int i = 0; i < numprocs; i++)
//                 // {
//                 //     printf("[%s] Client 0 --> Receive %d local changes from client %d\n", get_time().c_str(), client_local_change_size[i], i);
//                 // }
//                 log_file << graph.transfer_time << '\t' << graph.transfer_cost << '\t' << graph.movecost << '\t' << client_local_change_space_size / 2 << endl;

//                 printf("[LearningAutomaton] iteration : %d / %d\n", global_it + 1, global_iteration);
//                 printf("[LearningAutomaton] time : %f (%f)\n", graph.transfer_time, graph.transfer_time / origin_time);
//                 printf("[LearningAutomaton] cost : %f (%f)\n", graph.transfer_cost, graph.transfer_cost / origin_cost);
//                 printf("[LearningAutomaton] totalcost / budget : %f / %f\n", graph.movecost + global_movecost, Budget);
//                 if (auto_sampling_rate)
//                     printf("[LearningAutomaton] sampling rate : %f\n", sampling_rate);
//                 printf("[LearningAutomaton] change nums : %d\n", client_local_change_space_size / 2);
//                 printf("[LearningAutomaton] iteration use : %f s\n", iteration_use_overhead);
//                 graph.print(false);
//                 printf("\n\n");
//             }
//             auto training_end = chrono::steady_clock::now();
//             left_overhead = overhead_limit - ((double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000);
//             if(auto_sampling_rate)
//                 update_sampling_rate(global_it, samplingrate_overhead_rate, left_overhead);
//             // printf("[%d] sapling rate : %f, left overhead : %f\n", myid, sampling_rate, left_overhead);
//         }
//         if (myid == 0)
//         {
//             auto training_end = chrono::steady_clock::now();
//             graph.print(false);
//             printf("[LeanrningAutomaton] client training use : %f s\n", client_train_time);
//             printf("[LeanrningAutomaton] client update use : %f s\n", client_update_time);

//             printf("[LeanrningAutomaton] total training use : %f s\n", (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000);

//             log_file << "client training use :\t" << client_train_time << endl;
//             log_file << "client update use :\t" << client_update_time << endl;
//             log_file << "total training use :\t" << (double)std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_begin).count() / 1000 << endl;
//         }
//         MPI_Barrier(MPI_COMM_WORLD);
//     }
// };

#endif