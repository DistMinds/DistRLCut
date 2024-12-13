#include <bits/stdc++.h>
#include "mpi.h"

using namespace std;
struct bin_file_edge
{
    uint32_t u, v;
};

class CuSP
{
public:
    int worker_id;
    int worker_num;
    string NETWORK_FILE_NAME;
    string GRAPH_FILE_NAME;
    double UploadBandwidth = -1;
    double DownloadBandwidth = -1;
    double UploadPrice = -1;

    double gather_upload_wan = 0;   // 当前服务器在gather阶段上传使用的数据量
    double gather_download_wan = 0; // 当前服务器在gather阶段下载使用的数据量
    double apply_upload_wan = 0;    // 当前服务器在apply阶段上传使用的数据量
    double apply_download_wan = 0;  // 当前服务器在apply阶段下载使用的数据量

    vector<bin_file_edge> edges;

    vector<uint32_t> master_vertex; // global id

    vector<vector<uint32_t>> local_edges;
    unordered_map<uint32_t, uint32_t> mapped;
    vector<bool> is_heigh_degree_node;
    uint64_t local_edges_num = 0;

    unordered_set<uint32_t> upload_mirror;
    unordered_set<uint32_t> download_mirror;

    CuSP(int id, int num, string network_file_name, string graph_file_name)
    {
        worker_id = id;
        worker_num = num;
        NETWORK_FILE_NAME = network_file_name;
        GRAPH_FILE_NAME = graph_file_name;
    }
    void read_network_file() // 读取服务器文件内容
    {
        ifstream file;
        file.open(NETWORK_FILE_NAME, ios::in);

        if (!file.is_open())
        {
            cout << "Can't not open network file : " << NETWORK_FILE_NAME << endl;
            exit(-1);
        }

        stringstream ss;
        // ss << file.rdbuf();
        string s;

        int cnt = 0;

        while (getline(file, s))
        {
            ss.clear();
            ss << s;
            ss >> UploadBandwidth >> DownloadBandwidth >> UploadPrice;
            if (cnt == worker_id)
                break;
            ss.str("");
        }
        file.close();

        printf("Worker %d : UploadBandwidth %f\tDownloadBandwidth %f\tUploadPrice %f\n", worker_id, UploadBandwidth, DownloadBandwidth, UploadPrice);
    }
    void read_graph_file()
    {

        ifstream file;
        uint64_t read_begin_pos;
        uint64_t read_line_nums;
        if (worker_id == 0)
        {
            uint64_t edge_num;

            file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
            bin_file_edge header;
            file.read((char *)(&header), sizeof(header));

            file.read((char *)(&edge_num), sizeof(edge_num));
            cout << "[Graph State] edge num : " << edge_num << endl;

            file.close();

            uint64_t avg_edge_per_worker = edge_num / worker_num;
            vector<uint64_t> begin_pos(worker_num);
            vector<uint64_t> line_nums(worker_num);
            begin_pos[0] = 16;
            line_nums[0] = avg_edge_per_worker;
            for (int i = 1; i < worker_num; i++)
            {
                begin_pos[i] = begin_pos[i - 1] + 8 * avg_edge_per_worker;
                line_nums[i] = avg_edge_per_worker;
            }
            line_nums[worker_num - 1] += edge_num % avg_edge_per_worker;
            MPI_Scatter(begin_pos.data(), 1, MPI_UINT64_T, &read_begin_pos, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
            MPI_Scatter(line_nums.data(), 1, MPI_UINT64_T, &read_line_nums, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
        }
        else
        {
            MPI_Scatter(NULL, 1, MPI_UINT64_T, &read_begin_pos, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
            MPI_Scatter(NULL, 1, MPI_UINT64_T, &read_line_nums, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
        }
        edges.reserve(read_line_nums);

        file.open(GRAPH_FILE_NAME, ios::in | ios::binary);

        bin_file_edge tmp;
        uint32_t last_u = -1;

        if (worker_id == 0)
        {
            streampos pos(read_begin_pos);
            file.seekg(pos, ios::cur);
        }
        else
        {
            streampos pos(read_begin_pos - 8);
            file.seekg(pos, ios::cur);
            file.read((char *)(&tmp), sizeof(tmp));
            last_u = tmp.u;
        }
        int pass_edge = 0;
        while (true)
        {
            file.read((char *)(&tmp), sizeof(tmp));

            if (tmp.u != last_u)
            {
                file.seekg(-8, ios::cur);
                // cout << tmp.u << ' ' << last_u << ' ' << cnt << endl;
                break;
            }
            pass_edge++;
        }
        for (uint64_t i = 0; i < read_line_nums - pass_edge; i++)
        {
            file.read((char *)(&tmp), sizeof(tmp));
            edges.push_back(tmp);
            last_u = tmp.u;
        }
        // cout << worker_id << ' ' << last_u << " " << read_begin_pos << " " << read_line_nums << endl;
        if (worker_id != worker_num - 1)
        {
            while (true)
            {
                file.read((char *)(&tmp), sizeof(tmp));
                if (tmp.u != last_u)
                    break;
                edges.push_back(tmp);
            }
        }
        printf("Worker %d successful read %ld edges\n", worker_id, edges.size());
        file.close();
    }
    void read_file()
    {
        read_network_file();
        read_graph_file();
    }
    uint get_master(uint32_t id)
    {
        return id % worker_num;
    }
    uint get_edge_owner(bin_file_edge &x)
    {
        return get_master(x.v);
    }
    void master_assigned()
    {
        vector<unordered_set<uint32_t>> s(worker_num);
        for (auto &e : edges)
        {
            uint master = get_master(e.v);
            s[master].insert(e.v);
            master = get_master(e.u);
            s[master].insert(e.u);
        }
        vector<vector<uint32_t>> send(worker_num);
        for (int i = 0; i < s.size(); i++)
            send[i].assign(s[i].begin(), s[i].end());

        // vector<int> send_count(numprocs);
        // for (int i = 0; i < numprocs; i++)
        //     send_count[i] = send[i].size();

        // vector<unsigned int> oneD_vector;
        // for (auto &v : send)
        // {
        //     oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
        // }

        // // 合并成1维并发送
        // vector<unsigned int> recv = move(THB_Alltoallv(oneD_vector, send_count, numprocs, worker_id));
        vector<unsigned int> recv = move(THB_Alltoallv(send, worker_num, worker_id));

        for (auto &x : recv)
            s[worker_id].insert(x);
        master_vertex.assign(s[worker_id].begin(), s[worker_id].end());
        is_heigh_degree_node = vector<bool>(master_vertex.size(), false);

        printf("Worker %d get %d master\n", worker_id, master_vertex.size());
    }
    void edge_assign()
    {

        for (uint32_t i = 0; i < master_vertex.size(); i++)
        {
            mapped[master_vertex[i]] = i;
        }
        local_edges = vector<vector<uint32_t>>(master_vertex.size());

        vector<vector<uint32_t>> send(worker_num);

        for (auto &x : edges)
        {
            int worker = get_edge_owner(x);
            send[worker].push_back(x.u);
            send[worker].push_back(x.v);
        }
        // vector<int> send_count(numprocs);
        // for (int i = 0; i < numprocs; i++)
        //     send_count[i] = send[i].size();

        // vector<unsigned int> oneD_vector;
        // for (auto &v : send)
        // {
        //     oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
        // }

        // // 合并成1维并发送
        // // cout << oneD_vector[0] << ' ' << oneD_vector[1] << endl;
        // vector<unsigned int> recv = move(THB_Alltoallv(oneD_vector, send_count, numprocs, worker_id));

        vector<unsigned int> recv = move(THB_Alltoallv(send, worker_num, worker_id));

        for (uint i = 0; i < recv.size() / 2; i++)
        {
            uint32_t u = recv[i * 2];
            uint32_t mapped_id_v = mapped[recv[i * 2 + 1]];

            local_edges[mapped_id_v].push_back(u);
            if (local_edges[mapped_id_v].size() == THETA)
                is_heigh_degree_node[mapped_id_v] = true;
            local_edges_num++;
        }
        printf("Worker %d recv %ld edges\n", worker_id, local_edges_num);
    }
    void hybrid_cut()
    {

        vector<vector<uint32_t>> send(worker_num);

        for (uint i = 0; i < master_vertex.size(); i++)
        {
            if (is_heigh_degree_node[i])
            {
                uint global_id = master_vertex[i];
                vector<uint32_t> &e = local_edges[i];
                vector<bool> which_worker(worker_num, false);
                for (auto &u : e)
                {
                    int worker = get_master(u);
                    which_worker[worker] = true;
                    // send[worker].push_back(u);
                    
                }
                which_worker[worker_id] = false;
                for (int i = 0; i < which_worker.size(); i++)
                    if (which_worker[i])
                    {
                        gather_download_wan += DATA_UNIT;
                        send[i].push_back(global_id);
                    }
            }
        }

        vector<unsigned int> recv = move(THB_Alltoallv(send, worker_num, worker_id));

    cout << recv.size() << endl;;
        for (auto &v : recv)
        {
            gather_upload_wan += DATA_UNIT;
            // upload_mirror.insert(v);
        }
        // 

        for (uint i = 0; i < master_vertex.size(); i++)
        {
            if (!is_heigh_degree_node[i])
            {
                vector<uint32_t> &e = local_edges[i];
                for (auto &u : e)
                {
                    if(mapped.count(u) == 0)
                    download_mirror.insert(u);
                }
            }
        }
        for (auto &x : send)
            x.clear();
        for (auto &x : download_mirror)
        {
            int worker = get_master(x);
            send[worker].push_back(x);
            apply_download_wan += DATA_UNIT;
        }

        recv = move(THB_Alltoallv(send, worker_num, worker_id));
        for (auto &v : recv)
            apply_upload_wan += DATA_UNIT;
    }
    void print()
    {
        printf("Worker %d ---> gu %f\tgd %f\tau %f\tad %f\n", worker_id, gather_upload_wan, gather_download_wan, apply_upload_wan, apply_download_wan);
    }
};

// vector<unsigned int> THB_Alltoallv(const vector<unsigned int> &data, const vector<int> &send_counts, int numprocs, int myid)
// {
//     std::vector<int> send_displacements(numprocs, 0);
//     std::vector<int> recv_displacements(numprocs, 0);
//     std::vector<int> recv_counts(numprocs);

//     MPI_Alltoall(send_counts.data(), 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);

//     for (int i = 1; i < numprocs; ++i)
//     {
//         send_displacements[i] = send_displacements[i - 1] + send_counts[i - 1];
//         recv_displacements[i] = recv_displacements[i - 1] + recv_counts[i - 1];
//     }

//     unsigned int total_recv_count = recv_displacements[numprocs - 1] + recv_counts[numprocs - 1];

//     // Allocate space for the data
//     std::vector<unsigned int> recv_data(total_recv_count);

//     // Fill send_data with values for sending

//     // Perform the MPI_Alltoallv operation
//     MPI_Alltoallv(data.data(), send_counts.data(), send_displacements.data(), MPI_UINT32_T,
//                   recv_data.data(), recv_counts.data(), recv_displacements.data(), MPI_UINT32_T, MPI_COMM_WORLD);

//     return recv_data;
// }