#include <bits/stdc++.h>
#include <omp.h>
#include <unistd.h>
#include <mpi.h>

using namespace std;

#ifndef __GRAPH__
#define __GRAPH__

extern int THETA;             // 参数，决定一个顶点是否为High Degree
extern double DATA_UNIT;      // 参数，数据通信所消耗的基本数据流量大小，单位MB
extern double MOVE_DATA_UNIT; // 参数，移动一个顶点到其他服务器上消耗的流量，单位MB
extern bool directed_graph;
extern double dynamic_rate;
typedef unsigned int id_type; // 决定顶点id的数据类型

class Mirror
{
public:
    int in_degree = 0;
    int out_degree = 0;
    void add(int in, int out) // 使用镜像
    {

        in_degree += in;
        out_degree += out;
    }
    void clear()
    {
        in_degree = out_degree = 0;
    }
};

class Vertex // 顶点类
{
public:
    id_type id = -1;     // 顶点的唯一编号
    int current_dc = -1; // 当前所在的服务器
    int init_dc = -1;    // 初始所在的服务器 (mirror 则为master所在的dc)
    int in_degree = 0;   // 顶点的入度
    int out_degree = 0;  // 顶点的出度

    vector<id_type> *out_edge = &out_edge_inmem; // 顶点的出边邻居
    vector<id_type> out_edge_inmem;              // 顶点的出边邻居
    vector<id_type> in_edge;                     // 顶点的入边邻居
    // vector<id_type> out_edge;      // 顶点的出边邻居
    // vector<id_type> in_edge;       // 顶点的入边邻居
    bool is_high_degree = false; // 顶点是否为Hidh Degree

    // vector<Mirror> mirror_delta;
    // vector<Mirror> mirror;

    vector<unsigned int> worker_has_1hop;
};

class DataCenter // 服务器类
{
public:
    double UploadBandwidth = -1;   // 上传带宽
    double DownloadBandwidth = -1; // 下载带宽
    double UploadPrice = -1;       // 上传价格 ($/MB)

    double gather_upload_wan = 0;   // 当前服务器在gather阶段上传使用的数据量
    double gather_download_wan = 0; // 当前服务器在gather阶段下载使用的数据量
    double apply_upload_wan = 0;    // 当前服务器在apply阶段上传使用的数据量
    double apply_download_wan = 0;  // 当前服务器在apply阶段下载使用的数据量
    DataCenter(double up, double down, double price) : UploadBandwidth(up), DownloadBandwidth(down), UploadPrice(price){};
    void reset() // 重置服务器
    {
        gather_upload_wan = 0;
        gather_download_wan = 0;
        apply_upload_wan = 0;
        apply_download_wan = 0;
    }
};

double kBToGB(long kB)
{
    const double MB = 1024.0;
    const double GB = MB * 1024.0;

    return kB / GB;
}

template <typename T>
size_t getVectorMemoryUsage(const std::vector<T> &vec)
{
    return sizeof(vec) + vec.size() * sizeof(T);
}
// void THB_Allgatherv(const vector<unsigned int> &data, int numprocs, vector<unsigned int> &recv_data)
// {
//     vector<int> data_size_per_worker_to_send(numprocs);
//     int local_send_data_size = data.size();

//     vector<int> disp(numprocs);
//     // vector<unsigned int> recv_data;

//     MPI_Allgather(&local_send_data_size, 1, MPI_INT, data_size_per_worker_to_send.data(), 1, MPI_INT, MPI_COMM_WORLD);
//     for (int i = 1; i < numprocs; i++)
//         disp[i] = disp[i - 1] + data_size_per_worker_to_send[i - 1];
//     int total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), 0);
//     recv_data.resize(total_recv_data_size);

//     MPI_Allgatherv(data.data(), data.size(), MPI_UINT32_T, recv_data.data(), data_size_per_worker_to_send.data(), disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);
//     // return recv_data;
// }

// vector<unsigned int> THB_Allgatherv(const vector<unsigned int> &data, int numprocs)
// {
// const uint64_t MAX_SIZE = static_cast<uint64_t>(1) << 31; // 2GB
//     vector<uint64_t> data_size_per_worker_to_send(numprocs);
//     uint64_t local_send_data_size = data.size();

//     vector<uint64_t> disp64(numprocs);
//     vector<int> recv_counts(numprocs);
//     vector<int> disp(numprocs);
//     vector<unsigned int> recv_data;
//     // vector<unsigned int> temp_recv_data(MAX_SIZE * 2); // Temporary buffer for receiving data

//     MPI_Allgather(&local_send_data_size, 1, MPI_UINT64_T, data_size_per_worker_to_send.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);
//     for (int i = 1; i < numprocs; i++)
//         disp64[i] = disp64[i - 1] + data_size_per_worker_to_send[i - 1];
//     uint64_t total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), static_cast<uint64_t>(0));
//     recv_data.resize(total_recv_data_size);

//     uint64_t num_iterations = (total_recv_data_size + MAX_SIZE) / MAX_SIZE;
//     vector<uint64_t> had_recv(numprocs);

//     for (uint64_t iter = 0; iter < num_iterations; iter++) {
//         uint64_t send_offset = iter * (local_send_data_size / num_iterations);
//         uint64_t send_count = (iter == num_iterations - 1) ? local_send_data_size - send_offset : local_send_data_size / num_iterations;

//         // Copy relevant part of data to a new vector for sending
//         // vector<unsigned int> send_buffer(data.begin() + send_offset, data.begin() + send_offset + send_count);

//         for (int i = 0; i < numprocs; i++) {
//             uint64_t offset = iter * (data_size_per_worker_to_send[i] / num_iterations);
//             recv_counts[i] = (iter == num_iterations - 1) ? static_cast<int>(data_size_per_worker_to_send[i] - offset) : static_cast<int>(data_size_per_worker_to_send[i] / num_iterations);
//             disp[i] = (i == 0) ? 0 : recv_counts[i - 1] + disp[i - 1];
//         }
//         uint64_t temp_recv_data_size = accumulate(begin(recv_counts), end(recv_counts), static_cast<uint64_t>(0));
//         vector<unsigned int> temp_recv_data(temp_recv_data_size);
//         MPI_Allgatherv(data.data() + send_offset, static_cast<int>(send_count), MPI_UINT32_T,
//                        temp_recv_data.data(), recv_counts.data(),
//                        disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

//         // Copy from temp_recv_data to recv_data
//         for (int i = 0; i < numprocs; i++) {
//             std::copy(temp_recv_data.begin() + disp[i],
//                       temp_recv_data.begin() + disp[i] + recv_counts[i],
//                       recv_data.begin() + disp64[i] + had_recv[i]);
//             had_recv[i] += recv_counts[i];
//         }
//     }

//     // auto start = recv_data.begin() + disp64[myid];
//     // auto end = myid == numprocs - 1 ? recv_data.end() : recv_data.begin() + disp64[myid + 1];

//     // recv_data.erase(start, end);
//     return recv_data;
// }

// vector<unsigned int> THB_Allgatherv(const vector<unsigned int> &data, int numprocs, int myid)
// {
//     vector<int> data_size_per_worker_to_send(numprocs);
//     int local_send_data_size = data.size();

//     vector<int> disp(numprocs);
//     vector<unsigned int> recv_data;

//     MPI_Allgather(&local_send_data_size, 1, MPI_INT, data_size_per_worker_to_send.data(), 1, MPI_INT, MPI_COMM_WORLD);
//     for (int i = 1; i < numprocs; i++)
//         disp[i] = disp[i - 1] + data_size_per_worker_to_send[i - 1];
//     int total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), 0);
//     recv_data.resize(total_recv_data_size);

//     MPI_Allgatherv(data.data(), data.size(), MPI_UINT32_T, recv_data.data(), data_size_per_worker_to_send.data(), disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

//     auto start = recv_data.begin() + disp[myid];
//     auto end = myid == numprocs - 1 ? recv_data.end() : recv_data.begin() + disp[myid + 1];

//     recv_data.erase(start, end);
//     return recv_data;
// }

void THB_Allgatherv(const vector<unsigned int> &data, int numprocs, int myid, vector<unsigned int> &recv_data, bool with_local_data)
{
    const uint64_t MAX_SIZE = static_cast<uint64_t>(1) << 31; // 2GB
    vector<uint64_t> data_size_per_worker_to_send(numprocs);
    uint64_t local_send_data_size = data.size();

    vector<uint64_t> disp64(numprocs);
    vector<int> recv_counts(numprocs);
    vector<int> disp(numprocs);
    // vector<unsigned int> recv_data;
    // vector<unsigned int> temp_recv_data(MAX_SIZE * 2); // Temporary buffer for receiving data

    MPI_Allgather(&local_send_data_size, 1, MPI_UINT64_T, data_size_per_worker_to_send.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);
    for (int i = 1; i < numprocs; i++)
        disp64[i] = disp64[i - 1] + data_size_per_worker_to_send[i - 1];
    uint64_t total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), static_cast<uint64_t>(0));
    recv_data.resize(total_recv_data_size);

    uint64_t num_iterations = (total_recv_data_size + MAX_SIZE) / MAX_SIZE;

    if (num_iterations > 1)
    {
        // printf("[Allgatherv] Send multi message\n");
        vector<uint64_t> had_recv(numprocs);

        for (uint64_t iter = 0; iter < num_iterations; iter++)
        {
            uint64_t send_offset = iter * (local_send_data_size / num_iterations);
            uint64_t send_count = (iter == num_iterations - 1) ? local_send_data_size - send_offset : local_send_data_size / num_iterations;

            // Copy relevant part of data to a new vector for sending
            // vector<unsigned int> send_buffer(data.begin() + send_offset, data.begin() + send_offset + send_count);

            for (int i = 0; i < numprocs; i++)
            {
                uint64_t offset = iter * (data_size_per_worker_to_send[i] / num_iterations);
                recv_counts[i] = (iter == num_iterations - 1) ? static_cast<int>(data_size_per_worker_to_send[i] - offset) : static_cast<int>(data_size_per_worker_to_send[i] / num_iterations);
                disp[i] = (i == 0) ? 0 : recv_counts[i - 1] + disp[i - 1];
            }
            uint64_t temp_recv_data_size = accumulate(begin(recv_counts), end(recv_counts), static_cast<uint64_t>(0));
            vector<unsigned int> temp_recv_data(temp_recv_data_size);
            MPI_Allgatherv(data.data() + send_offset, static_cast<int>(send_count), MPI_UINT32_T,
                           temp_recv_data.data(), recv_counts.data(),
                           disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

            // Copy from temp_recv_data to recv_data
            for (int i = 0; i < numprocs; i++)
            {
#pragma omp parallel for
                for (uint64_t j = 0; j < recv_counts[i]; j++)
                    recv_data[disp64[i] + had_recv[i] + j] = temp_recv_data[disp[i] + j];

                // std::copy(temp_recv_data.begin() + disp[i],
                //           temp_recv_data.begin() + disp[i] + recv_counts[i],
                //           recv_data.begin() + disp64[i] + had_recv[i]);
                had_recv[i] += recv_counts[i];
            }
        }
    }
    else
    {
        // printf("[Allgatherv] Send single message\n");
        for (int i = 0; i < numprocs; i++)
        {
            recv_counts[i] = static_cast<int>(data_size_per_worker_to_send[i]);
            disp[i] = (i == 0) ? 0 : disp[i - 1] + recv_counts[i - 1];
        }

        MPI_Allgatherv(data.data(), data.size(), MPI_UINT32_T, recv_data.data(), recv_counts.data(), disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);
    }

    if (!with_local_data)
    {
        auto start = recv_data.begin() + disp64[myid];
        auto end = myid == numprocs - 1 ? recv_data.end() : recv_data.begin() + disp64[myid + 1];

        recv_data.erase(start, end);
    }
    // return recv_data;
}

void THB_Allgatherv(const vector<unsigned int> &data, int numprocs, int myid, vector<unsigned int> &recv_data, vector<uint64_t> &disp64, bool with_local_data)
{
    const uint64_t MAX_SIZE = static_cast<uint64_t>(1) << 31; // 2GB
    vector<uint64_t> data_size_per_worker_to_send(numprocs);
    uint64_t local_send_data_size = data.size();

    // vector<uint64_t> disp64(numprocs);
    vector<int> recv_counts(numprocs);
    vector<int> disp(numprocs);
    // vector<unsigned int> recv_data;
    // vector<unsigned int> temp_recv_data(MAX_SIZE * 2); // Temporary buffer for receiving data

    MPI_Allgather(&local_send_data_size, 1, MPI_UINT64_T, data_size_per_worker_to_send.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);
    for (int i = 1; i < numprocs; i++)
        disp64[i] = disp64[i - 1] + data_size_per_worker_to_send[i - 1];
    uint64_t total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), static_cast<uint64_t>(0));
    recv_data.resize(total_recv_data_size);

    uint64_t num_iterations = (total_recv_data_size + MAX_SIZE) / MAX_SIZE;

    if (num_iterations > 1)
    {
        // printf("[Allgatherv] Send multi message\n");
        vector<uint64_t> had_recv(numprocs);

        for (uint64_t iter = 0; iter < num_iterations; iter++)
        {
            uint64_t send_offset = iter * (local_send_data_size / num_iterations);
            uint64_t send_count = (iter == num_iterations - 1) ? local_send_data_size - send_offset : local_send_data_size / num_iterations;

            // Copy relevant part of data to a new vector for sending
            // vector<unsigned int> send_buffer(data.begin() + send_offset, data.begin() + send_offset + send_count);

            for (int i = 0; i < numprocs; i++)
            {
                uint64_t offset = iter * (data_size_per_worker_to_send[i] / num_iterations);
                recv_counts[i] = (iter == num_iterations - 1) ? static_cast<int>(data_size_per_worker_to_send[i] - offset) : static_cast<int>(data_size_per_worker_to_send[i] / num_iterations);
                disp[i] = (i == 0) ? 0 : recv_counts[i - 1] + disp[i - 1];
            }
            uint64_t temp_recv_data_size = accumulate(begin(recv_counts), end(recv_counts), static_cast<uint64_t>(0));
            vector<unsigned int> temp_recv_data(temp_recv_data_size);
            MPI_Allgatherv(data.data() + send_offset, static_cast<int>(send_count), MPI_UINT32_T,
                           temp_recv_data.data(), recv_counts.data(),
                           disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

            // Copy from temp_recv_data to recv_data
            for (int i = 0; i < numprocs; i++)
            {
#pragma omp parallel for
                for (uint64_t j = 0; j < recv_counts[i]; j++)
                    recv_data[disp64[i] + had_recv[i] + j] = temp_recv_data[disp[i] + j];

                // std::copy(temp_recv_data.begin() + disp[i],
                //           temp_recv_data.begin() + disp[i] + recv_counts[i],
                //           recv_data.begin() + disp64[i] + had_recv[i]);
                had_recv[i] += recv_counts[i];
            }
        }
    }
    else
    {
        // printf("[Allgatherv] Send single message\n");
        for (int i = 0; i < numprocs; i++)
        {
            recv_counts[i] = static_cast<int>(data_size_per_worker_to_send[i]);
            disp[i] = (i == 0) ? 0 : disp[i - 1] + recv_counts[i - 1];
        }

        MPI_Allgatherv(data.data(), data.size(), MPI_UINT32_T, recv_data.data(), recv_counts.data(), disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);
    }

    if (!with_local_data)
    {
        auto start = recv_data.begin() + disp64[myid];
        auto end = myid == numprocs - 1 ? recv_data.end() : recv_data.begin() + disp64[myid + 1];

        recv_data.erase(start, end);
    }
    // return recv_data;
}

// vector<unsigned int> THB_Allgatherv(const vector<unsigned int> &data, int numprocs, int myid, vector<unsigned int> &data)
// {
//     const uint64_t MAX_SIZE = static_cast<uint64_t>(1) << 31; // 2GB
//     vector<uint64_t> data_size_per_worker_to_send(numprocs);
//     uint64_t local_send_data_size = data.size();

//     vector<uint64_t> disp64(numprocs);
//     vector<int> recv_counts(numprocs);
//     vector<int> disp(numprocs);
//     vector<unsigned int> recv_data;
//     // vector<unsigned int> temp_recv_data(MAX_SIZE * 2); // Temporary buffer for receiving data

//     MPI_Allgather(&local_send_data_size, 1, MPI_UINT64_T, data_size_per_worker_to_send.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);
//     for (int i = 1; i < numprocs; i++)
//         disp64[i] = disp64[i - 1] + data_size_per_worker_to_send[i - 1];
//     uint64_t total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), static_cast<uint64_t>(0));
//     recv_data.resize(total_recv_data_size);

//     uint64_t num_iterations = (total_recv_data_size + MAX_SIZE) / MAX_SIZE;

//     for (uint64_t iter = 0; iter < num_iterations; iter++) {
//         uint64_t send_offset = iter * (local_send_data_size / num_iterations);
//         uint64_t send_count = (iter == num_iterations - 1) ? local_send_data_size - send_offset : local_send_data_size / num_iterations;

//         // Copy relevant part of data to a new vector for sending
//         // vector<unsigned int> send_buffer(data.begin() + send_offset, data.begin() + send_offset + send_count);

//         for (int i = 0; i < numprocs; i++) {
//             uint64_t offset = iter * (data_size_per_worker_to_send[i] / num_iterations);
//             recv_counts[i] = (iter == num_iterations - 1) ? static_cast<int>(data_size_per_worker_to_send[i] - offset) : static_cast<int>(data_size_per_worker_to_send[i] / num_iterations);
//             disp[i] = (i == 0) ? 0 : recv_counts[i - 1] + disp[i - 1];
//         }
//         uint64_t temp_recv_data_size = accumulate(begin(recv_counts), end(recv_counts), static_cast<uint64_t>(0));
//         vector<unsigned int> temp_recv_data(temp_recv_data_size);
//         MPI_Allgatherv(data.data() + send_offset, static_cast<int>(send_count), MPI_UINT32_T,
//                        temp_recv_data.data(), recv_counts.data(),
//                        disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

//         // Copy from temp_recv_data to recv_data
//         for (int i = 0; i < numprocs; i++) {
//             std::copy(temp_recv_data.begin() + disp[i],
//                       temp_recv_data.begin() + disp[i] + recv_counts[i],
//                       recv_data.begin() + disp64[i] + send_offset);
//         }
//     }

//     auto start = recv_data.begin() + disp64[myid];
//     auto end = myid == numprocs - 1 ? recv_data.end() : recv_data.begin() + disp64[myid + 1];

//     recv_data.erase(start, end);
//     return recv_data;
// }

vector<unsigned int> THB_Alltoallv(const vector<unsigned int> &data, const vector<int> &send_counts, int numprocs, int myid)
{
    std::vector<int> send_displacements(numprocs, 0);
    std::vector<int> recv_displacements(numprocs, 0);
    std::vector<int> recv_counts(numprocs);

    MPI_Alltoall(send_counts.data(), 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);

    for (int i = 1; i < numprocs; ++i)
    {
        send_displacements[i] = send_displacements[i - 1] + send_counts[i - 1];
        recv_displacements[i] = recv_displacements[i - 1] + recv_counts[i - 1];
    }

    unsigned int total_recv_count = recv_displacements[numprocs - 1] + recv_counts[numprocs - 1];

    // Allocate space for the data
    std::vector<unsigned int> recv_data(total_recv_count);

    // Fill send_data with values for sending

    // Perform the MPI_Alltoallv operation
    MPI_Alltoallv(data.data(), send_counts.data(), send_displacements.data(), MPI_UINT32_T,
                  recv_data.data(), recv_counts.data(), recv_displacements.data(), MPI_UINT32_T, MPI_COMM_WORLD);

    return recv_data;
}

vector<unsigned int> THB_Alltoallv(const vector<unsigned int> &data, const vector<uint64_t> &send_counts, int numprocs, int myid, vector<unsigned int> &recv_data)
{
    const uint64_t MAX_SIZE = static_cast<uint64_t>(1) << 30; // 2GB
    vector<uint64_t> recv_disp64(numprocs);
    vector<uint64_t> send_disp64(numprocs);
    std::vector<uint64_t> recv_counts(numprocs);

    MPI_Alltoall(send_counts.data(), 1, MPI_UINT64_T, recv_counts.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);

    for (int i = 1; i < numprocs; ++i)
    {
        recv_disp64[i] = recv_disp64[i - 1] + recv_counts[i - 1];
        send_disp64[i] = send_disp64[i - 1] + send_counts[i - 1];
    }

    uint64_t total_recv_count = recv_disp64[numprocs - 1] + recv_counts[numprocs - 1];
    // std::vector<unsigned int> recv_data(total_recv_count);
    recv_data.resize(total_recv_count);

    uint64_t local_num_iterations = (total_recv_count) / MAX_SIZE + 1;
    uint64_t num_iterations;
    MPI_Allreduce(&local_num_iterations, &num_iterations, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD);

    if (num_iterations > 1)
    {
        // printf("[Alltoall] Send multi message\n");
        uint chunk_size = data.size() / num_iterations;

        vector<uint64_t> had_recv(numprocs);

        for (int iter = 0; iter < num_iterations; iter++)
        {
            uint64_t offset = iter * chunk_size;
            uint iter_send_size = (iter == numprocs - 1) ? data.size() - offset : chunk_size;

            vector<int> iter_send_disp(numprocs);
            vector<int> iter_send_counts(numprocs);

            for (int i = 1; i < numprocs; i++)
            {
                iter_send_disp[i] = static_cast<int>(min(send_disp64[i] - offset, (uint64_t)iter_send_size));
                iter_send_counts[i - 1] = iter_send_disp[i] - iter_send_disp[i - 1];
            }
            iter_send_counts[numprocs - 1] = iter_send_size - iter_send_disp[numprocs - 1];

            vector<int> iter_recv_disp(numprocs);
            std::vector<int> iter_recv_counts(numprocs);
            MPI_Alltoall(iter_send_counts.data(), 1, MPI_INT32_T, iter_recv_counts.data(), 1, MPI_INT32_T, MPI_COMM_WORLD);

            int iter_recv_size = accumulate(begin(iter_recv_counts), end(iter_recv_counts), 0);
            vector<uint> tmp_recv_data(iter_recv_size);

            for (int i = 1; i < numprocs; i++)
                iter_recv_disp[i] = iter_recv_disp[i - 1] + recv_counts[i - 1];

            MPI_Alltoallv(data.data() + offset, iter_send_counts.data(), iter_send_disp.data(), MPI_UINT32_T,
                          tmp_recv_data.data(), iter_recv_counts.data(), iter_recv_disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);
            for (int i = 0; i < numprocs; i++)
            {
#pragma omp parallel for
                for (uint64_t j = 0; j < recv_counts[i]; j++)
                    recv_data[recv_disp64[i] + had_recv[i] + j] = tmp_recv_data[iter_recv_disp[i] + j];

                // std::copy(temp_recv_data.begin() + disp[i],
                //           temp_recv_data.begin() + disp[i] + recv_counts[i],
                //           recv_data.begin() + disp64[i] + had_recv[i]);
                had_recv[i] += recv_counts[i];
            }
        }
    }
    else
    {
        // printf("[Alltoall] Send single message\n");
        vector<int> recv_disp(numprocs);
        vector<int> send_disp(numprocs);
        vector<int> send_counts_32(numprocs);
        vector<int> recv_counts_32(numprocs);

        for (int i = 0; i < numprocs; i++)
        {
            recv_disp[i] = static_cast<int>(recv_disp64[i]);
            send_disp[i] = static_cast<int>(send_disp64[i]);
            recv_counts_32[i] = static_cast<int>(recv_counts[i]);
            send_counts_32[i] = static_cast<int>(send_counts[i]);
        }
        MPI_Alltoallv(data.data(), send_counts_32.data(), send_disp.data(), MPI_UINT32_T,
                      recv_data.data(), recv_counts_32.data(), recv_disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);
    }

    return recv_data;
}

vector<unsigned int> THB_Alltoallv(const vector<vector<uint>> &data, int numprocs, int myid)
{
    vector<uint64_t> send_count(numprocs);
    for (int i = 0; i < numprocs; i++)
        send_count[i] = data[i].size();

    vector<unsigned int> oneD_vector;
    for (auto &v : data)
    {
        oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
    }

    // 合并成1维并发送
    // cout << oneD_vector[0] << ' ' << oneD_vector[1] << endl;
    // return move(THB_Alltoallv(oneD_vector, send_count, numprocs, myid));
    vector<uint> recv;
    THB_Alltoallv(oneD_vector, send_count, numprocs, myid, recv);
    return recv;
}

void large_data_comm_test(int numprocs, int myid)
{
    vector<vector<uint>> send(5, vector<uint>(268435456L * 1));
    // printf("Worker %d create %f GB vector successfully\n", myid, kBToGB(getVectorMemoryUsage(send) / 1024));
    // vector<uint> recv = move(THB_Allgatherv(send, numprocs, myid));
    vector<uint> recv = move(THB_Alltoallv(send, numprocs, myid));
    printf("Worker %d recv %f GB successfully\n", myid, kBToGB(getVectorMemoryUsage(recv) / 1024));
}

class Graph // 图类
{
public:
    int worker_id;

    uint32_t vertex_num = 0; // 顶点数量
    uint64_t edge_num = 0;   // 边数量
    int DC_num = 0;          // 地理式分布的服务器数量

    double transfer_time; // 完成一次通信的时间
    double transfer_cost; // 完成一次通信消耗的价格
    double movecost = 0;  // 移动顶点消耗的价格

    vector<Vertex> vertex; // 所有的顶点
    vector<DataCenter> DC; // 所有的服务器
    // vector<vector<Mirror>> mirror; // 所有的镜像，服务器数量×顶点数量
    string GRAPH_FILE_NAME;   // 图文件名
    string NETWORK_FILE_NAME; // 服务器文件名

    unordered_map<id_type, id_type> mapped; // 顶点映射
    id_type related_v_num;

    uint64_t file_lines; // 图文件长度
    atomic_uint64_t high_degree_node_num = {0};

    bool display_graph_state;

    unordered_set<id_type> agent_in_subgraph;
    vector<id_type> agent_in_subgraph_vector;

    const int COUPLE_SIZE = 2;
    int numprocs;

    vector<bool> change_tag;

    vector<unsigned int> global_dc_vertex_num;

    vector<vector<Mirror>> mirror_delta;
    vector<vector<Mirror>> mirror;

    struct bin_file_edge
    {
        uint32_t u, v;
    };

    vector<bin_file_edge> dynamic_edges_save;
    vector<id_type> dynamic_vertex;

    Graph(string graph_file_name, string network_file_name, int id, int proc, bool display = false)
    {
        worker_id = id;
        GRAPH_FILE_NAME = graph_file_name;
        NETWORK_FILE_NAME = network_file_name;
        display_graph_state = display;
        numprocs = proc;
    }
    Graph() {}
    // Graph(const Graph &x)
    // {
    //     worker_id = x.worker_id;
    //     GRAPH_FILE_NAME = x.GRAPH_FILE_NAME;
    //     NETWORK_FILE_NAME = x.NETWORK_FILE_NAME;
    // }
    Graph &operator=(const Graph &x)
    {
        // Implementation of copy assignment operator
        worker_id = x.worker_id;
        GRAPH_FILE_NAME = x.GRAPH_FILE_NAME;
        NETWORK_FILE_NAME = x.NETWORK_FILE_NAME;
        display_graph_state = x.display_graph_state;
        numprocs = x.numprocs;
        DC_num = x.DC_num;
        return *this;
    }
    void comm_update_init_DC()
    {
        // printf("numprocs : %d\n", numprocs);
        vector<unsigned int> tmp(agent_in_subgraph_vector.size());
        iota(tmp.begin(), tmp.end(), 0);
        apply_subgraph_agent_DC(update_global_change(tmp), COUPLE_SIZE);
    }
    /*void get_comm_dest()
    {
        vector<int> data_size_per_worker_to_send(numprocs);
        vector<unsigned int> send(vertex.size() - agent_in_subgraph_vector.size());

#pragma omp parallel for
        for (id_type i = 0; i < send.size(); i++)
        {
            id_type v = agent_in_subgraph_vector.size() + i;
            send[i] = vertex[v].id;
        }

        int local_send_data_size = send.size();

        vector<int> disp(numprocs);
        vector<unsigned int> recv_data;

        MPI_Allgather(&local_send_data_size, 1, MPI_INT, data_size_per_worker_to_send.data(), 1, MPI_INT, MPI_COMM_WORLD);
        for (int i = 1; i < numprocs; i++)
            disp[i] = disp[i - 1] + data_size_per_worker_to_send[i - 1];
        int total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), 0);
        recv_data.resize(total_recv_data_size);

        MPI_Allgatherv(send.data(), send.size(), MPI_UINT32_T, recv_data.data(), data_size_per_worker_to_send.data(), disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

        vector<omp_lock_t> write(agent_in_subgraph.size());
        for (auto &x : write)
            omp_init_lock(&x);

#pragma omp parallel for
        for (unsigned int i = 0; i < recv_data.size(); i++)
        {
            id_type id = recv_data[i];
            if (agent_in_subgraph.count(id))
            {
                id_type mapped_id = mapped[id];
                int which_worker = -1;
                for (int index = numprocs - 1; index >= 0; index--)
                    if (i >= disp[index])
                    {
                        which_worker = index;
                        break;
                    }
                omp_set_lock(&write[mapped_id]);
                vertex[mapped_id].worker_has_1hop.push_back(which_worker);

                omp_unset_lock(&write[mapped_id]);
            }
        }
        unsigned int sum = 0;
#pragma omp parallel for reduction(+ : sum)
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            sum += vertex[i].worker_has_1hop.size();
        }
        printf("Worker %d avg send to %f worker\n", worker_id, 1. * sum / agent_in_subgraph_vector.size());

        local_send_data_size = agent_in_subgraph_vector.size();
        MPI_Allgather(&local_send_data_size, 1, MPI_INT, data_size_per_worker_to_send.data(), 1, MPI_INT, MPI_COMM_WORLD);
        disp[0] = 0;
        for (int i = 1; i < numprocs; i++)
            disp[i] = disp[i - 1] + data_size_per_worker_to_send[i - 1];
        total_recv_data_size = accumulate(begin(data_size_per_worker_to_send), end(data_size_per_worker_to_send), 0);
        recv_data.resize(total_recv_data_size);
        MPI_Allgatherv(agent_in_subgraph_vector.data(), agent_in_subgraph_vector.size(), MPI_UINT32_T, recv_data.data(), data_size_per_worker_to_send.data(), disp.data(), MPI_UINT32_T, MPI_COMM_WORLD);

#pragma omp parallel for
        for (unsigned int i = 0; i < recv_data.size(); i++)
        {
            id_type id = recv_data[i];
            if (mapped.count(id) && !agent_in_subgraph.count(id))
            {
                id_type mapped_id = mapped[id];
                int which_worker = -1;
                for (int index = numprocs - 1; index >= 0; index--)
                    if (i >= disp[index])
                    {
                        which_worker = index;
                        break;
                    }
                vertex[mapped_id].init_dc = which_worker;
            }
        }
    }*/
    void get_comm_dest()
    {
        printf("Worker %d begins get_comm_dest()\n", worker_id);
        vector<int> data_size_per_worker_to_send(numprocs);
        vector<unsigned int> send(vertex.size() - agent_in_subgraph_vector.size());

#pragma omp parallel for
        for (id_type i = 0; i < send.size(); i++)
        {
            id_type v = agent_in_subgraph_vector.size() + i;
            send[i] = vertex[v].id;
        }

        vector<uint64_t> disp(numprocs);
        vector<unsigned int> recv_data;

        THB_Allgatherv(send, numprocs, worker_id, recv_data, disp, true);

        vector<omp_lock_t> write(agent_in_subgraph.size());
        for (auto &x : write)
            omp_init_lock(&x);

#pragma omp parallel for
        for (uint64_t i = 0; i < recv_data.size(); i++)
        {
            id_type id = recv_data[i];
            if (agent_in_subgraph.count(id))
            {
                id_type mapped_id = mapped[id];
                int which_worker = -1;
                for (int index = numprocs - 1; index >= 0; index--)
                    if (i >= disp[index])
                    {
                        which_worker = index;
                        break;
                    }
                omp_set_lock(&write[mapped_id]);
                vertex[mapped_id].worker_has_1hop.push_back(which_worker);

                omp_unset_lock(&write[mapped_id]);
            }
        }
        unsigned int sum = 0;
#pragma omp parallel for reduction(+ : sum)
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            sum += vertex[i].worker_has_1hop.size();
        }
        printf("Worker %d avg send to %f worker\n", worker_id, 1. * sum / agent_in_subgraph_vector.size());

        THB_Allgatherv(agent_in_subgraph_vector, numprocs, worker_id, recv_data, disp, true);

#pragma omp parallel for
        for (uint64_t i = 0; i < recv_data.size(); i++)
        {
            id_type id = recv_data[i];
            if (mapped.count(id) && !agent_in_subgraph.count(id))
            {
                id_type mapped_id = mapped[id];
                int which_worker = -1;
                for (int index = numprocs - 1; index >= 0; index--)
                    if (i >= disp[index])
                    {
                        which_worker = index;
                        break;
                    }
                vertex[mapped_id].init_dc = which_worker;
            }
        }
    }

    // change_id : global_id
    /*
    vector<unsigned int> update_global_change(const vector<id_type> &change_id)
    {

        int send_local_change_size = change_id.size() * COUPLE_SIZE;
        vector<unsigned int> send_change(send_local_change_size);

#pragma omp parallel for
        for (id_type i = 0; i < change_id.size(); i++)
        {
            id_type agent_id = change_id[i];
            id_type mapped_id = mapped[agent_id];
            send_change[i * COUPLE_SIZE] = agent_id;
            send_change[i * COUPLE_SIZE + 1] = vertex[mapped_id].current_dc;
        }

        return THB_Allgatherv(send_change, numprocs);
    }*/
    // change_id : local id
    vector<unsigned int> update_global_change(const vector<id_type> &change_id)
    {
        vector<omp_lock_t> write(numprocs);
        for (auto &x : write)
            omp_init_lock(&x);

        vector<vector<unsigned int>> send(numprocs);
#pragma omp parallel for
        for (id_type i = 0; i < change_id.size(); i++)
        {
            Vertex &v = vertex[change_id[i]];
            for (auto &j : v.worker_has_1hop)
            {
                omp_set_lock(&write[j]);
                send[j].emplace_back(v.id);
                send[j].emplace_back(v.current_dc);
                omp_unset_lock(&write[j]);
            }
        }
        // vector<int> send_count(numprocs);
        // for (int i = 0; i < numprocs; i++)
        //     send_count[i] = send[i].size();

        // vector<unsigned int> oneD_vector;
        // for (auto &v : send)
        // {
        //     oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
        // }

        return THB_Alltoallv(send, numprocs, worker_id);
    }
    void apply_subgraph_agent_DC(const vector<unsigned int> &global_change, int couple_size)
    {
#pragma omp parallel for
        for (id_type i = 0; i < (global_change.size() / couple_size); i++)
        {
            id_type agent_id = global_change[i * couple_size];
            unsigned int dc = global_change[i * couple_size + 1];

            id_type mapped_id = mapped[agent_id];
            vertex[mapped_id].current_dc = dc;
        }
    }
    void comm_update_high_degree_node()
    {
        vector<unsigned int> recv;
        THB_Allgatherv(get_high_degree_nodes(), numprocs, worker_id, recv, true);
        apply_subgraph_high_degree_node(recv);
    }
    void apply_subgraph_high_degree_node(const vector<unsigned int> &high_degree_node_id)
    {
        int cnt = 0;
        // printf("----------=-=-=-=%d\n", high_degree_node_id.size());
#pragma omp parallel for
        for (id_type i = 0; i < high_degree_node_id.size(); i++)
        {
            id_type agent_id = high_degree_node_id[i];
            if (mapped.count(agent_id) && !agent_in_subgraph.count(agent_id))
            {
                id_type mapped_id = mapped[agent_id];
                vertex[mapped_id].is_high_degree = true;
                cnt++;
            }
        }
        printf("Worker %d successful update %d high degree node\n", worker_id, cnt);
    }
    void resize_out_edge()
    {
        if (!directed_graph)
            return;
#pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            Vertex &v = vertex[i];
            vector<id_type> new_out_edge;
            for (auto &e : *v.out_edge)
                if (vertex[e].is_high_degree)
                    new_out_edge.emplace_back(e);
            v.out_edge->clear();
            v.out_edge->assign(new_out_edge.begin(), new_out_edge.end());
        }
    }
    vector<unsigned int> get_high_degree_nodes()
    {
        vector<unsigned int> high_degree_nodes;
        high_degree_nodes.reserve(high_degree_node_num);
        omp_lock_t write_lock;
        omp_init_lock(&write_lock);

#pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)

            if (vertex[i].is_high_degree)
            {
                omp_set_lock(&write_lock);
                high_degree_nodes.emplace_back(vertex[i].id);
                omp_unset_lock(&write_lock);
            }
        printf("worker %d will send %d high degree nodes id\n", worker_id, high_degree_nodes.size());
        return high_degree_nodes;
    }

    void read_graph_file()
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
            file.close();
            if (display_graph_state)
                cout << "[Graph State] Binary files detected ^_^ " << endl;
            read_graph_file_bin();
        }
        else
        {
            file.close();
            if (display_graph_state)
                cout << "[Graph State] Binary files not detected T T " << header.u << " " << header.v << endl;
            read_graph_file_vertex();
            read_graph_file_edge();
        }
    }

    void read_graph_file_bin()
    {
        ifstream file;

        file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
        if (!file.is_open())
        {
            cout << "Can't not open graph file : " << GRAPH_FILE_NAME << endl;
            exit(-1);
        }
        bin_file_edge header;
        file.read((char *)(&header), sizeof(header));
        vertex_num = header.v;
        if (display_graph_state)
            cout << "[Graph State] vertex num : " << vertex_num << endl;

        vertex = vector<Vertex>(vertex_num);

        file.read((char *)(&edge_num), sizeof(edge_num));
        if (display_graph_state)
            cout << "[Graph State] edge num : " << edge_num << endl;

        file.close();

        int tn = omp_get_max_threads() - 1;

        vector<uint64_t> cur_line(tn, 0);
        // cout << tn << endl;
        // uint32_t tn = 1;
        vector<uint64_t> line_per_thread(tn);
        for (int i = 0; i < tn; i++)
            line_per_thread[i] = edge_num / tn;
        line_per_thread[tn - 1] += edge_num % tn;

        vector<omp_lock_t> lock(vertex_num);
        for (auto &x : lock)
            omp_init_lock(&x);

            // int sum = 0;
            // for(auto x : line_per_thread)   {cout << x << endl; sum += x;}
            // cout << sum << endl;

            /*
                    vector<ifstream> thread_file(tn);

                    for(int i = 0; i < thread_file.size(); i++)
                    {
                        thread_file[i].open(GRAPH_FILE_NAME, ios::in | ios::binary);
                        if(!thread_file[i].is_open())   cout << "error" << endl;
                        // thread_file[i].seekg(16 + i * edge_num / tn);
                        thread_file[i].seekg(16);
                    }
            */

#pragma omp parallel
        {
            // cout << omp_get_thread_num() << endl;
            if (omp_get_thread_num() != tn)
            {
                bin_file_edge tmp;
                /*
                while (file.read((char *)(&tmp), sizeof(tmp)))
                {
                    id_type source = tmp.u;
                    id_type dest = tmp.v;

                    cur_line++;

                    vertex[source].out_edge.push_back(dest);
                    vertex[source].out_degree++;

                    vertex[dest].in_edge.push_back(source);
                    vertex[dest].in_degree++;

                    if (vertex[dest].in_degree == THETA)
                        vertex[dest].is_high_degree = true, high_degree_node_num++;
                }*/
                int tnn = omp_get_thread_num();
                ifstream thread_file;
                thread_file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
                streampos pos(16 + tnn * 8 * (edge_num / tn));
                // cout << pos << endl;
                thread_file.seekg(pos, ios::cur);

                // cout << tnn << " " << line_per_thread[tnn] << endl;
                for (uint64_t i = 0; i < line_per_thread[tnn]; i++)
                {
                    // cout << tnn << " " << thread_file.tellg() << endl;
                    thread_file.read((char *)(&tmp), sizeof(tmp));

                    // file.read((char *)(&tmp), sizeof(tmp));
                    // cout << tnn << endl;

                    id_type source = tmp.u;
                    id_type dest = tmp.v;
                    // cout << source << " " << dest << endl;

                    cur_line[tnn]++;

                    omp_set_lock(&lock[source]);
                    vertex[source].out_edge->push_back(dest);
                    vertex[source].out_degree++;

                    omp_unset_lock(&lock[source]);

                    omp_set_lock(&lock[dest]);
                    vertex[dest].in_edge.push_back(source);
                    vertex[dest].in_degree++;

                    if (vertex[dest].in_degree == THETA)
                        vertex[dest].is_high_degree = true, high_degree_node_num++;

                    omp_unset_lock(&lock[dest]);
                }
                // cout << cur_line << endl;
                // cur = vertex_num;
                thread_file.close();
            }
            else
            {
                if (display_graph_state)
                {
                    int line_num = 10;
                    uint64_t cl = 0;
                    do
                    {
                        cl = 0;
                        for (auto &x : cur_line)
                            cl += x;
                        int schedule = 100 * cl / edge_num;

                        printf("Loading edges : %d%% \t[", schedule);
                        for (int i = 0; i < line_num; i++)
                        {
                            if (i < schedule * line_num / 100)
                                cout << "*";
                            else
                                cout << " ";
                        }
                        cout << "]";
                        fflush(stdout);
                        usleep(10000);
                        cout << '\r';

                        // for (int i = 0; i < 200; i++)
                        //     cout << "\b";
                        fflush(stdout);
                    } while (cl != edge_num);
                    cout << endl
                         << "[Graph State] edge num : " << edge_num << endl;
                }
            }
        }
    }
    void read_graph_file_bin(unordered_set<id_type> &agent_in_subgraph)
    {
        ifstream file;

        file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
        if(!file.is_open())
        {
            printf("can't not open file : %s\n", GRAPH_FILE_NAME);
            exit(-1);
        }
        bin_file_edge header;
        file.read((char *)(&header), sizeof(header));
        vertex_num = header.v;
        if (display_graph_state)
            cout << "[Graph State] vertex num : " << vertex_num << endl;

        vertex.clear();
        const int extend_rate = 1;
        vertex.reserve(agent_in_subgraph.size() * extend_rate);

        file.read((char *)(&edge_num), sizeof(edge_num));
        if (display_graph_state)
            cout << "[Graph State] edge num : " << edge_num << endl;
        file.close();

        int tn = omp_get_max_threads() - 1;

        vector<uint64_t> cur_line(tn, 0);

        vector<uint64_t> line_per_thread(tn);
        for (int i = 0; i < tn; i++)
            line_per_thread[i] = edge_num / tn;
        line_per_thread[tn - 1] += edge_num % tn;

        omp_lock_t map_cnt_lock;
        omp_init_lock(&map_cnt_lock);

        // for (auto &x : agent_in_subgraph_vector)
        // {
        //     Vertex tmp;
        //     tmp.id = x;
        //     tmp.mirror.resize(DC_num);

        //     tmp.mirror_delta.resize(DC_num);
        //     vertex.emplace_back(tmp);

        //     mapped[x] = related_v_num++;
        // }

        vertex.resize(agent_in_subgraph_vector.size());
        for (auto &x : agent_in_subgraph_vector)
        {
            vertex[related_v_num].id = x;
            mapped[x] = related_v_num++;
            // vertex[related_v_num].out_edge = &vertex[related_v_num].out_edge_inmem;
        }

        print_RAM_size();

        vector<vector<id_type>> cross_neighbour_id(tn);

#pragma omp parallel
        {
            // cout << omp_get_thread_num() << endl;
            if (omp_get_thread_num() != tn)
            {
                bin_file_edge tmp;

                int tnn = omp_get_thread_num();
                ifstream thread_file;
                thread_file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
                streampos pos(16 + tnn * 8 * (edge_num / tn));

                // cout << pos << endl;
                thread_file.seekg(pos, ios::cur);

                // if(display_graph_state)
                //     cout << line_per_thread[tnn] << endl;

                for (uint64_t i = 0; i < line_per_thread[tnn]; i++)
                {

                    thread_file.read((char *)(&tmp), sizeof(tmp));

                    id_type source = tmp.u;
                    id_type dest = tmp.v;

                    cur_line[tnn]++;

                    if (agent_in_subgraph.count(source) && !agent_in_subgraph.count(dest))
                        cross_neighbour_id[tnn].emplace_back(dest);
                    if (agent_in_subgraph.count(dest) && !agent_in_subgraph.count(source))
                        cross_neighbour_id[tnn].emplace_back(source);
                }

                // cout << cur_line << endl;
                // cur = vertex_num;
                thread_file.close();
            }
            else
            {
                if (display_graph_state)
                {
                    int line_num = 10;
                    uint64_t cl = 0;
                    do
                    {
                        cl = 0;
                        for (auto &x : cur_line)
                            cl += x;
                        int schedule = 100 * cl / edge_num;

                        printf("Mapping index : %d%% \t[", schedule);
                        for (int i = 0; i < line_num; i++)
                        {
                            if (i < schedule * line_num / 100)
                                cout << "*";
                            else
                                cout << " ";
                        }
                        cout << "]";
                        fflush(stdout);
                        usleep(10000);
                        cout << '\r';

                        fflush(stdout);
                    } while (cl != edge_num);
                    cout << endl
                         << "[Graph State] edge num : " << edge_num << endl;
                }
            }
        }
        for (auto &a : cross_neighbour_id)
            for (auto &b : a)
                if (mapped.count(b) == 0)
                {
                    mapped[b] = related_v_num++;
                }
        // vertex.reserve(related_v_num);
        vertex.resize(related_v_num);
#pragma omp parallel for
        for (uint32_t i = 0; i < vertex.size(); i++)
            vertex[i].out_edge = &vertex[i].out_edge_inmem;

        vector<omp_lock_t> lock(related_v_num);
#pragma omp parallel for
        for (uint32_t i = 0; i < lock.size(); i++)
            omp_init_lock(&lock[i]);

#pragma omp parallel for
        for (int i = 0; i < cross_neighbour_id.size(); i++)
        {
            for (id_type j = 0; j < cross_neighbour_id[i].size(); j++)
            {
                id_type id = cross_neighbour_id[i][j];
                id_type mapped_id = mapped[id];

                omp_set_lock(&lock[mapped_id]);
                if (vertex[mapped_id].id == -1)
                {
                    vertex[mapped_id].id = id;
                    // vertex[mapped_id].mirror.resize(DC_num);
                    // vertex[mapped_id].mirror_delta.resize(DC_num);
                }
                omp_unset_lock(&lock[mapped_id]);
            }
        }
        mirror = vector<vector<Mirror>>(DC_num, vector<Mirror>(related_v_num));
        mirror_delta = vector<vector<Mirror>>(DC_num, vector<Mirror>(related_v_num));
        ;

        print_RAM_size();
        // cout << 1111 << endl;
        change_tag.resize(related_v_num, false);

        for (auto &x : cur_line)
            x = 0;

        // return;
        uint64_t local_edges_num = 0;
#pragma omp parallel reduction(+ : local_edges_num)
        {
            // cout << omp_get_thread_num() << endl;
            if (omp_get_thread_num() != tn)
            {
                bin_file_edge tmp;

                int tnn = omp_get_thread_num();
                ifstream thread_file;
                thread_file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
                streampos pos(16 + tnn * 8 * (edge_num / tn));

                // cout << pos << endl;
                thread_file.seekg(pos, ios::cur);

                // if(display_graph_state)
                //     cout << line_per_thread[tnn] << endl;

                for (uint64_t i = 0; i < line_per_thread[tnn]; i++)
                {

                    thread_file.read((char *)(&tmp), sizeof(tmp));

                    id_type source = tmp.u;
                    id_type dest = tmp.v;

                    cur_line[tnn]++;

                    if (!(agent_in_subgraph.count(source) || agent_in_subgraph.count(dest)))
                        continue;

                    id_type mapped_source = mapped[source];
                    id_type mapped_dest = mapped[dest];

                    local_edges_num++;

                    // 锁的是local id
                    if (agent_in_subgraph.count(source))
                    {
                        omp_set_lock(&lock[mapped_source]);
                        // 边使用的是local id
                        // vertex[mapped_source].out_edge = &vertex[mapped_source].out_edge_inmem;
                        // printf("%d %d\n", vertex[mapped_source].out_edge->size(), mapped_source);
                        vertex[mapped_source].out_edge->push_back(mapped_dest);
                        // cout << vertex[mapped_source].out_edge << endl;
                        vertex[mapped_source].out_degree++;
                        omp_unset_lock(&lock[mapped_source]);
                    }
                    if (agent_in_subgraph.count(dest))
                    {
                        omp_set_lock(&lock[mapped_dest]);
                        vertex[mapped_dest].in_edge.push_back(mapped_source);
                        vertex[mapped_dest].in_degree++;

                        if (vertex[mapped_dest].in_degree == THETA)
                            vertex[mapped_dest].is_high_degree = true, high_degree_node_num++;

                        omp_unset_lock(&lock[mapped_dest]);
                    }

                    // omp_unset_lock(&map_cnt_lock);
                }

                // cout << cur_line << endl;
                // cur = vertex_num;
                thread_file.close();
            }
            else
            {
                if (display_graph_state)
                {
                    int line_num = 10;
                    uint64_t cl = 0;
                    do
                    {
                        cl = 0;
                        for (auto &x : cur_line)
                            cl += x;
                        int schedule = 100 * cl / edge_num;

                        printf("Loading edges : %d%% \t[", schedule);
                        for (int i = 0; i < line_num; i++)
                        {
                            if (i < schedule * line_num / 100)
                                cout << "*";
                            else
                                cout << " ";
                        }
                        cout << "]";
                        fflush(stdout);
                        usleep(10000);
                        cout << '\r';

                        // for (int i = 0; i < 200; i++)
                        //     cout << "\b";
                        fflush(stdout);
                    } while (cl != edge_num);
                    cout << endl
                         << "[Graph State] edge num : " << edge_num << endl;
                }
            }
        }
        if (!directed_graph)
        {
#pragma omp parallel for
            for (uint32_t i = 0; i < vertex.size(); i++)
            {
                vertex[i].in_edge.insert(vertex[i].in_edge.end(), vertex[i].out_edge->begin(), vertex[i].out_edge->end());
                sort(vertex[i].in_edge.begin(), vertex[i].in_edge.end());
                vertex[i].in_edge.erase(unique(vertex[i].in_edge.begin(), vertex[i].in_edge.end()), vertex[i].in_edge.end());
                vertex[i].in_degree = vertex[i].in_edge.size();
                if (vertex[i].in_degree >= THETA && vertex[i].is_high_degree == false)
                    vertex[i].is_high_degree = true, high_degree_node_num++;
                vertex[i].out_degree = vertex[i].in_degree;
                // vertex[i].out_edge = &vertex[i].in_edge, vertex[i].in_degree = vertex[i].out_degree;
                vertex[i].out_edge->clear();
                vertex[i].out_edge = &vertex[i].in_edge;
            }
        }
        // printf("Worker %d : %d vertex size\n", worker_id, vertex.size());
        // #pragma omp parallel for reduction(+ : high_degree_node_num)
        //         for (id_type i = 0; i < agent_in_subgraph.size(); i++)
        //         {
        //             if (vertex[i].in_degree >= THETA)
        //                 vertex[i].is_high_degree = true, high_degree_node_num += 1;
        //         }
        printf("Worker %d finished read edges  : %ld \n", worker_id, local_edges_num);
        printf("Worker %d's high degree node num : %d\n", worker_id, high_degree_node_num.load());
        print_RAM_size();
    }
    void read_graph_file_bin(unordered_set<id_type> &agent_in_subgraph, double dynamic_rate)
    {
        ifstream file;

        file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
        bin_file_edge header;
        file.read((char *)(&header), sizeof(header));
        vertex_num = header.v;
        if (display_graph_state)
            cout << "[Graph State] vertex num : " << vertex_num << endl;

        vertex.clear();
        const int extend_rate = 1;
        vertex.reserve(agent_in_subgraph.size() * extend_rate);

        file.read((char *)(&edge_num), sizeof(edge_num));
        if (display_graph_state)
            cout << "[Graph State] edge num : " << edge_num << endl;
        file.close();

        int tn = omp_get_max_threads() - 1;

        vector<uint64_t> cur_line(tn, 0);

        vector<uint64_t> line_per_thread(tn);
        for (int i = 0; i < tn; i++)
            line_per_thread[i] = edge_num / tn;
        line_per_thread[tn - 1] += edge_num % tn;

        omp_lock_t map_cnt_lock;
        omp_init_lock(&map_cnt_lock);

        // for (auto &x : agent_in_subgraph_vector)
        // {
        //     Vertex tmp;
        //     tmp.id = x;
        //     tmp.mirror.resize(DC_num);

        //     tmp.mirror_delta.resize(DC_num);
        //     vertex.emplace_back(tmp);

        //     mapped[x] = related_v_num++;
        // }

        vertex.resize(agent_in_subgraph_vector.size());
        for (auto &x : agent_in_subgraph_vector)
        {
            vertex[related_v_num].id = x;
            mapped[x] = related_v_num++;
            // vertex[related_v_num].out_edge = &vertex[related_v_num].out_edge_inmem;
        }

        print_RAM_size();

        vector<vector<id_type>> cross_neighbour_id(tn);

#pragma omp parallel
        {
            // cout << omp_get_thread_num() << endl;
            if (omp_get_thread_num() != tn)
            {
                bin_file_edge tmp;

                int tnn = omp_get_thread_num();
                ifstream thread_file;
                thread_file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
                streampos pos(16 + tnn * 8 * (edge_num / tn));

                // cout << pos << endl;
                thread_file.seekg(pos, ios::cur);

                // if(display_graph_state)
                //     cout << line_per_thread[tnn] << endl;

                for (uint64_t i = 0; i < line_per_thread[tnn]; i++)
                {

                    thread_file.read((char *)(&tmp), sizeof(tmp));

                    id_type source = tmp.u;
                    id_type dest = tmp.v;

                    cur_line[tnn]++;

                    if (agent_in_subgraph.count(source) && !agent_in_subgraph.count(dest))
                        cross_neighbour_id[tnn].emplace_back(dest);
                    if (agent_in_subgraph.count(dest) && !agent_in_subgraph.count(source))
                        cross_neighbour_id[tnn].emplace_back(source);
                }

                // cout << cur_line << endl;
                // cur = vertex_num;
                thread_file.close();
            }
            else
            {
                if (display_graph_state)
                {
                    int line_num = 10;
                    uint64_t cl = 0;
                    do
                    {
                        cl = 0;
                        for (auto &x : cur_line)
                            cl += x;
                        int schedule = 100 * cl / edge_num;

                        printf("Mapping index : %d%% \t[", schedule);
                        for (int i = 0; i < line_num; i++)
                        {
                            if (i < schedule * line_num / 100)
                                cout << "*";
                            else
                                cout << " ";
                        }
                        cout << "]";
                        fflush(stdout);
                        usleep(10000);
                        cout << '\r';

                        fflush(stdout);
                    } while (cl != edge_num);
                    cout << endl
                         << "[Graph State] edge num : " << edge_num << endl;
                }
            }
        }
        for (auto &a : cross_neighbour_id)
            for (auto &b : a)
                if (mapped.count(b) == 0)
                {
                    mapped[b] = related_v_num++;
                }
        // vertex.reserve(related_v_num);
        vertex.resize(related_v_num);
#pragma omp parallel for
        for (uint32_t i = 0; i < vertex.size(); i++)
            vertex[i].out_edge = &vertex[i].out_edge_inmem;

        vector<omp_lock_t> lock(related_v_num);
#pragma omp parallel for
        for (uint32_t i = 0; i < lock.size(); i++)
            omp_init_lock(&lock[i]);

#pragma omp parallel for
        for (int i = 0; i < cross_neighbour_id.size(); i++)
        {
            for (id_type j = 0; j < cross_neighbour_id[i].size(); j++)
            {
                id_type id = cross_neighbour_id[i][j];
                id_type mapped_id = mapped[id];

                omp_set_lock(&lock[mapped_id]);
                if (vertex[mapped_id].id == -1)
                {
                    vertex[mapped_id].id = id;
                    // vertex[mapped_id].mirror.resize(DC_num);
                    // vertex[mapped_id].mirror_delta.resize(DC_num);
                }
                omp_unset_lock(&lock[mapped_id]);
            }
        }
        mirror = vector<vector<Mirror>>(DC_num, vector<Mirror>(related_v_num));
        mirror_delta = vector<vector<Mirror>>(DC_num, vector<Mirror>(related_v_num));
        ;

        print_RAM_size();
        // cout << 1111 << endl;
        change_tag.resize(related_v_num, false);

        for (auto &x : cur_line)
            x = 0;

        vector<vector<bin_file_edge>> dynamic_thread_save(tn);

        // return;
        uint64_t local_edges_num = 0;
#pragma omp parallel reduction(+ : local_edges_num)
        {
            // cout << omp_get_thread_num() << endl;
            if (omp_get_thread_num() != tn)
            {
                bin_file_edge tmp;

                int tnn = omp_get_thread_num();
                ifstream thread_file;
                thread_file.open(GRAPH_FILE_NAME, ios::in | ios::binary);
                streampos pos(16 + tnn * 8 * (edge_num / tn));

                // cout << pos << endl;
                thread_file.seekg(pos, ios::cur);

                // if(display_graph_state)
                //     cout << line_per_thread[tnn] << endl;

                unsigned int rand_seed = time(NULL) ^ tnn;
                unsigned int dynamic = dynamic_rate * RAND_MAX;

                for (uint64_t i = 0; i < line_per_thread[tnn]; i++)
                {

                    thread_file.read((char *)(&tmp), sizeof(tmp));

                    id_type source = tmp.u;
                    id_type dest = tmp.v;

                    cur_line[tnn]++;

                    if (!(agent_in_subgraph.count(source) || agent_in_subgraph.count(dest)))
                        continue;

                    if (rand_r(&rand_seed) <= dynamic)
                    {
                        dynamic_thread_save[tnn].push_back({source, dest});
                        // if(tnn != 0)    cout << 1111111111 << endl;
                        continue;
                    }

                    id_type mapped_source = mapped[source];
                    id_type mapped_dest = mapped[dest];

                    local_edges_num++;

                    // 锁的是local id
                    if (agent_in_subgraph.count(source))
                    {
                        omp_set_lock(&lock[mapped_source]);
                        // 边使用的是local id
                        // vertex[mapped_source].out_edge = &vertex[mapped_source].out_edge_inmem;
                        // printf("%d %d\n", vertex[mapped_source].out_edge->size(), mapped_source);
                        vertex[mapped_source].out_edge->push_back(mapped_dest);
                        // cout << vertex[mapped_source].out_edge << endl;
                        vertex[mapped_source].out_degree++;
                        omp_unset_lock(&lock[mapped_source]);
                    }
                    if (agent_in_subgraph.count(dest))
                    {
                        omp_set_lock(&lock[mapped_dest]);
                        vertex[mapped_dest].in_edge.push_back(mapped_source);
                        vertex[mapped_dest].in_degree++;

                        if (vertex[mapped_dest].in_degree == THETA)
                            vertex[mapped_dest].is_high_degree = true, high_degree_node_num++;

                        omp_unset_lock(&lock[mapped_dest]);
                    }

                    // omp_unset_lock(&map_cnt_lock);
                }

                // cout << cur_line << endl;
                // cur = vertex_num;
                thread_file.close();
            }
            else
            {
                if (display_graph_state)
                {
                    int line_num = 10;
                    uint64_t cl = 0;
                    do
                    {
                        cl = 0;
                        for (auto &x : cur_line)
                            cl += x;
                        int schedule = 100 * cl / edge_num;

                        printf("Loading edges : %d%% \t[", schedule);
                        for (int i = 0; i < line_num; i++)
                        {
                            if (i < schedule * line_num / 100)
                                cout << "*";
                            else
                                cout << " ";
                        }
                        cout << "]";
                        fflush(stdout);
                        usleep(10000);
                        cout << '\r';

                        // for (int i = 0; i < 200; i++)
                        //     cout << "\b";
                        fflush(stdout);
                    } while (cl != edge_num);
                    cout << endl
                         << "[Graph State] edge num : " << edge_num << endl;
                }
            }
        }
        if (!directed_graph)
        {
#pragma omp parallel for
            for (uint32_t i = 0; i < vertex.size(); i++)
            {
                vertex[i].in_edge.insert(vertex[i].in_edge.end(), vertex[i].out_edge->begin(), vertex[i].out_edge->end());
                sort(vertex[i].in_edge.begin(), vertex[i].in_edge.end());
                vertex[i].in_edge.erase(unique(vertex[i].in_edge.begin(), vertex[i].in_edge.end()), vertex[i].in_edge.end());
                vertex[i].in_degree = vertex[i].in_edge.size();
                if (vertex[i].in_degree >= THETA && vertex[i].is_high_degree == false)
                    vertex[i].is_high_degree = true, high_degree_node_num++;
                vertex[i].out_degree = vertex[i].in_degree;
                // vertex[i].out_edge = &vertex[i].in_edge, vertex[i].in_degree = vertex[i].out_degree;
                vertex[i].out_edge->clear();
                vertex[i].out_edge = &vertex[i].in_edge;
            }
        }
        dynamic_edges_save.clear();
        for (auto &x : dynamic_thread_save)
        {
            // if (worker_id == 0)  cout << x.size() << endl;
            dynamic_edges_save.insert(dynamic_edges_save.end(), x.begin(), x.end());
        }
        // printf("Worker %d : %d vertex size\n", worker_id, vertex.size());
        // #pragma omp parallel for reduction(+ : high_degree_node_num)
        //         for (id_type i = 0; i < agent_in_subgraph.size(); i++)
        //         {
        //             if (vertex[i].in_degree >= THETA)
        //                 vertex[i].is_high_degree = true, high_degree_node_num += 1;
        //         }
        uint64_t totol_local_edges = local_edges_num + dynamic_edges_save.size();

        printf("Worker %d finished read edges  : %ld / %ld(dynamic rate : %f) \n", worker_id, local_edges_num, totol_local_edges, 1. * local_edges_num / totol_local_edges);
        printf("Worker %d's high degree node num : %d\n", worker_id, high_degree_node_num.load());
        print_RAM_size();
    }
    void dynamic_reloads()
    {

        vector<omp_lock_t> lock(related_v_num);
        for (auto &x : lock)
            omp_init_lock(&x);

        vector<id_type> re_high_degree;
        omp_lock_t re_high;
        omp_init_lock(&re_high);

        vector<bool> is_dynamic(agent_in_subgraph.size(), false);

#pragma omp parallel for
        for (uint64_t i = 0; i < dynamic_edges_save.size(); i++)
        {
            id_type source = dynamic_edges_save[i].u;
            id_type dest = dynamic_edges_save[i].v;

            id_type mapped_source = mapped[source];
            id_type mapped_dest = mapped[dest];

            if (agent_in_subgraph.count(source))
            {
                omp_set_lock(&lock[mapped_source]);
                // 边使用的是local id
                vertex[mapped_source].out_edge->push_back(mapped_dest);
                vertex[mapped_source].out_degree++;
                is_dynamic[mapped_source] = true;
                omp_unset_lock(&lock[mapped_source]);
            }
            if (agent_in_subgraph.count(dest))
            {
                omp_set_lock(&lock[mapped_dest]);
                // 边使用的是local id
                vertex[mapped_dest].in_edge.push_back(mapped_source);
                vertex[mapped_dest].in_degree++;

                is_dynamic[mapped_dest] = true;
                // cout << 1 <<endl;
                if (vertex[mapped_dest].in_degree == THETA)
                {
                    vertex[mapped_dest].is_high_degree = true, high_degree_node_num++;
                    omp_set_lock(&re_high);
                    re_high_degree.push_back(dest);
                    omp_unset_lock(&re_high);
                }

                omp_unset_lock(&lock[mapped_dest]);
            }
        }
        if (!directed_graph)
        {
#pragma omp parallel for
            for (uint32_t i = 0; i < vertex.size(); i++)
            {
                // vertex[i].in_edge.insert(vertex[i].in_edge.end(), vertex[i].out_edge->begin(), vertex[i].out_edge->end());
                sort(vertex[i].in_edge.begin(), vertex[i].in_edge.end());
                vertex[i].in_edge.erase(unique(vertex[i].in_edge.begin(), vertex[i].in_edge.end()), vertex[i].in_edge.end());
                vertex[i].in_degree = vertex[i].in_edge.size();
                if (vertex[i].in_degree >= THETA && vertex[i].is_high_degree == false)
                {
                    vertex[i].is_high_degree = true, high_degree_node_num++;
                    omp_set_lock(&re_high);
                    re_high_degree.push_back(i);
                    omp_unset_lock(&re_high);
                }
                vertex[i].out_degree = vertex[i].in_degree;
                // vertex[i].out_edge = &vertex[i].in_edge, vertex[i].in_degree = vertex[i].out_degree;
                // vertex[i].out_edge->clear();
                // vertex[i].out_edge = &vertex[i].in_edge;
            }
        }

        for (uint i = 0; i < agent_in_subgraph.size(); i++)
            if (is_dynamic[i])
                dynamic_vertex.push_back(i);

        printf("Worker %d has %ld high degree node\n", worker_id, high_degree_node_num.load());
        // printf("Worker %d successfully reloads %ld edges\n", worker_id, reload_edges_num);

        vector<unsigned int> recv;
        THB_Allgatherv(re_high_degree, numprocs, worker_id, recv, true);
        apply_subgraph_high_degree_node(recv);
    }
    void read_graph_file_vertex() // 读取图文件
    {
        ifstream file;
        file.open(GRAPH_FILE_NAME, ios::in);
        if (!file.is_open())
        {
            cout << "Can't not open graph file : " << GRAPH_FILE_NAME << endl;
            exit(-1);
        }

        file_lines = 0;
        stringstream ss;
        // ss << file.rdbuf();
        string s;

        id_type max_id = 0;
        id_type tmp;

        id_type mapped_count = 0;

        cout << "Finding max vertex id...\r" << endl;

        // 顶点的连续映射
        while (getline(file, s))
        {
            file_lines++;
            ss.clear();
            ss << s;
            ss >> tmp;
            if (!mapped.count(tmp))
                mapped[tmp] = mapped_count++;
            max_id = max(mapped[tmp], max_id);
            ss >> tmp;

            if (!mapped.count(tmp))
                mapped[tmp] = mapped_count++;
            max_id = max(mapped[tmp], max_id);
            ss.str("");
        }

        // vertex_num = max_id + 1;
        vertex_num = mapped.size();

        // 输出图顶点的数据情况
        cout << "[Graph State] max vertex id : " << max_id << endl;
        cout << "[Graph State] real vertex num : " << mapped.size() << endl;

        vertex = vector<Vertex>(vertex_num);
    }

    void read_graph_file_edge() // 读取图边的数据
    {
        ifstream file;
        file.open(GRAPH_FILE_NAME, ios::in);

        if (!file.is_open())
        {
            cout << "Can't not open graph file : " << GRAPH_FILE_NAME << endl;
            exit(-1);
        }

        stringstream ss;
        // ss << file.rdbuf();
        string s;

        id_type max_id = -1;
        id_type source, dest;
        id_type cur_line = 0;
        // 一个线程负责读取边，一个线程负责输出当前读取情况到屏幕
#pragma omp parallel num_threads(2) shared(cur_line)
        {

            if (omp_get_thread_num() == 0)
            {
                while (getline(file, s))
                {
                    ss.clear();
                    ss << s;
                    ss >> source >> dest;

                    ss.str("");

                    source = mapped[source];
                    dest = mapped[dest];

                    cur_line++;

                    edge_num++;

                    if (source == dest)
                        continue;

                    vertex[source].out_edge->push_back(dest);
                    vertex[source].out_degree++;

                    vertex[dest].in_edge.push_back(source);
                    vertex[dest].in_degree++;

                    if (vertex[dest].in_degree == THETA)
                        vertex[dest].is_high_degree = true, high_degree_node_num++;
                }
                // cur = vertex_num;
            }
            else
            {
                int line_num = 10;
                while (cur_line != file_lines)
                {
                    int schedule = 100 * cur_line / file_lines + 1;
                    printf("Loading edge : %d%% \t[", schedule);
                    for (int i = 0; i < line_num; i++)
                    {
                        if (i < schedule * line_num / 100)
                            cout << "*";
                        else
                            cout << " ";
                    }
                    cout << "]";
                    fflush(stdout);
                    usleep(10000);
                    cout << '\r';

                    // for (int i = 0; i < 200; i++)
                    //     cout << "\b";
                    fflush(stdout);
                }
                cout << endl
                     << "[Graph State] edge num : " << edge_num << endl;
            }
        }
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

        double UploadBandwidth = -1;
        double DownloadBandwidth = -1;
        double UploadPrice = -1;

        while (getline(file, s))
        {
            ss.clear();
            ss << s;
            ss >> UploadBandwidth >> DownloadBandwidth >> UploadPrice;
            DC_num++;
            DC.emplace_back(DataCenter(UploadBandwidth, DownloadBandwidth, UploadPrice));
            ss.str("");
        }
        if (display_graph_state)
            cout << "[Graph State] DC num : " << DC_num << endl;
        file.close();
    }

    void read_file() // 读取所有文件
    {
        read_graph_file();
        // read_graph_file_vertex();
        // read_graph_file_edge();
        read_network_file();

        // 建立镜像
        // vector<Mirror> m(vertex_num);
        // for (int i = 0; i < DC_num; i++)
        //     mirror.push_back(m);
    }
    void read_file(vector<id_type> &agent_vector, bool dynamic = false)
    {
        agent_in_subgraph_vector = agent_vector;
        agent_in_subgraph = unordered_set<id_type>(agent_vector.begin(), agent_vector.end());
        mapped.clear();
        related_v_num = 0;
        high_degree_node_num = 0;
        read_network_file();
        if (dynamic)
            read_graph_file_bin(agent_in_subgraph, dynamic_rate);
        else
            read_graph_file_bin(agent_in_subgraph);
    }
    void paritition_to_DC(int mode)
    {
        enum paritition_mode
        {
            HASH,
            RANGE,
            RANDOM,
            BINFILE,
            ASCIIFILE
        };
        switch (mode)
        {
        case HASH:
        {
            if (display_graph_state)
                cout << "Partitioning graph using HASH" << endl;
#pragma omp parallel for
            for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
            {
                vertex[i].init_dc = vertex[i].current_dc = vertex[i].id % DC_num;
            }
            break;
        }
        case RANGE:
        {
            if (display_graph_state)
                cout << "Partitioning graph using RANGE" << endl;
            id_type vertex_per_worker = vertex_num / DC_num;
            cout << worker_id << " : vertex per worker = " << vertex_per_worker << ' ' << vertex_num << ' ' << DC_num << endl;
#pragma omp parallel for
            for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
            {
                vertex[i].init_dc = vertex[i].current_dc = min(vertex[i].id / vertex_per_worker, (id_type)DC_num - 1);
            }
            cout << "Worker " << worker_id << " Partitioning graph using RANGE Finished" << endl;
            break;
        }
        case RANDOM:
        {
            if (display_graph_state)
                cout << "Partitioning graph using RANDOM" << endl;
            srand(time(NULL));
            vector<unsigned int> thread_seed(omp_get_max_threads());
            for (auto &x : thread_seed)
                x = rand();

#pragma omp parallel for
            for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
            {
                int k = omp_get_thread_num();
                vertex[i].init_dc = vertex[i].current_dc = rand_r(&thread_seed[k]) % DC_num;
            }
            break;
        }
        case BINFILE:
        {
            if (display_graph_state)
                cout << "Partitioning graph using FILE" << endl;
            string filename = GRAPH_FILE_NAME;
            size_t lastDotPos = filename.find_last_of(".");
            filename = filename.substr(0, lastDotPos) + ".part";

            ifstream in(filename, std::ios::binary);
            if (!in.is_open())
            {
                printf("can't open partition file : %s\n", filename.c_str());
                exit(-1);
            }
            uint head[2];

            in.read((char *)(&head), sizeof(uint) * 2);
            if (head[0] != 1145141919)
            {
                printf("not steander partition file : %s\n", filename.c_str());
                exit(-1);
            }
            if (head[1] != vertex_num)
            {
                printf("error partition file : %s\n", filename.c_str());
                exit(-1);
            }
            vector<char> dc_tmp(vertex_num);
            in.read(dc_tmp.data(), sizeof(char) * vertex_num);

#pragma omp parallel for
            for (uint i = 0; i < agent_in_subgraph.size(); i++)
            {
                uint id = vertex[i].id;
                char file_dc = dc_tmp[id];
                vertex[i].current_dc = vertex[i].init_dc = file_dc;
            }
            printf("Finished file partition\n");
            break;
        }
        case ASCIIFILE:
        {
            if (display_graph_state)
                cout << "Partitioning graph using FILE" << endl;
            string filename = GRAPH_FILE_NAME;
            size_t lastDotPos = filename.find_last_of(".");
            filename = filename + ".parts.5";

            ifstream in(filename);
            if (!in.is_open())
            {
                printf("can't open partition file : %s\n", filename.c_str());
                exit(-1);
            }

            string str;
            int i = 0;
            vector<int> dc_tmp;
            while (getline(in, str))
            {
                int file_dc = stoi(str);
                dc_tmp.push_back(file_dc);
            }
            printf("%d %d\n", dc_tmp.size(), vertex_num);
            assert(dc_tmp.size() == vertex_num);

#pragma omp parallel for
            for (uint i = 0; i < agent_in_subgraph.size(); i++)
            {
                uint id = vertex[i].id;
                char file_dc = (dc_tmp[id] + 1) % DC_num;
                vertex[i].current_dc = vertex[i].init_dc = file_dc;
            }
            printf("Finished file partition\n");
            break;
        }

        default:
            exit(-1);
        }

        // vector<int> client_local_change_size(numprocs);
        // MPI_Allgather(&send_local_change_size, 1, MPI_INT, client_local_change_size.data(), 1, MPI_INT, MPI_COMM_WORLD);
    }

    void print_cross_dc_edge_num() // 输出跨dc的边的数量
    {
        long long cross_dc_edge_num = 0;
#pragma omp parallel for
        for (id_type i = 0; i < vertex_num; i++)
        {
            // cout << omp_get_num_threads() << endl;
            int dc = vertex[i].current_dc;
            for (auto x : vertex[i].in_edge)
                if (dc != vertex[x].current_dc)
#pragma omp atomic
                    cross_dc_edge_num++;
        }
        if (display_graph_state)
            printf("Cross DC edge num : %lld (%f of total)\n", cross_dc_edge_num, 1. * cross_dc_edge_num / edge_num);
    }

    void reset_DC()
    {
        for (int i = 0; i < DC_num; i++)
            DC[i].reset();
    }
    void reset_Mirror()
    {
        for (auto &x : mirror)
        {
#pragma omp parallel for
            for (id_type i = 0; i < x.size(); i++)
                x[i].clear();
        }
    }
    vector<unsigned int> get_DC_vertex_num()
    {
        vector<atomic_uint> local_dc_vertex_num(DC_num);
        vector<unsigned int> global_dc_vertex_num(DC_num);

#pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            local_dc_vertex_num[vertex[i].current_dc]++;
        }
        for (int i = 0; i < DC_num; i++)
            MPI_Allreduce(&local_dc_vertex_num[i], &global_dc_vertex_num[i], 1, MPI_UINT32_T, MPI_SUM, MPI_COMM_WORLD);
        return global_dc_vertex_num;
    }
    void print_in_worker0()
    {
        global_dc_vertex_num = move(get_DC_vertex_num());
        if (worker_id == 0)
        {
            printf("[Graph status] : Worker %d\n", worker_id);
            printf("agent in worker : %d\n", agent_in_subgraph.size());
            printf("related agent :  %d\n", related_v_num);
            for (int dc = 0; dc < DC_num; dc++)
            {
                printf("Worker %d ---> DC %d (%d vertices): gu %f\tgd %f\tau %f\tad %f\n", worker_id, dc, global_dc_vertex_num[dc], DC[dc].gather_upload_wan, DC[dc].gather_download_wan, DC[dc].apply_upload_wan, DC[dc].apply_download_wan);
            }
            printf("Data transfer time : %f\n", transfer_time);
            printf("Data transfer cost : %f\n", transfer_cost);
        }
    }
    void print()
    {
        printf("[Graph status] : Worker %d\n", worker_id);
        printf("agent in worker : %d\n", agent_in_subgraph.size());
        printf("related agent :  %d\n", related_v_num);
        for (int dc = 0; dc < DC_num; dc++)
        {
            printf("Worker %d ---> DC %d (%d vertices): gu %f\tgd %f\tau %f\tad %f\n", worker_id, dc, global_dc_vertex_num[dc], DC[dc].gather_upload_wan, DC[dc].gather_download_wan, DC[dc].apply_upload_wan, DC[dc].apply_download_wan);
        }
        printf("Data transfer time : %f\n", transfer_time);
        printf("Data transfer cost : %f\n", transfer_cost);
    }
    void print_in_order()
    {
        // 按顺序执行函数
        global_dc_vertex_num = move(get_DC_vertex_num());
        for (int i = 0; i < numprocs; i++)
        {
            if (worker_id == i)
            {
                // 执行函数
                print();
            }
            // 等待其他rank完成
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    void print_RAM_size()
    {
        std::ifstream statusFile("/proc/self/status");
        std::string line;

        for (int i = 0; i < numprocs; i++)
        {
            if (worker_id == i)
            {
                std::cout << "Worker " << worker_id << std::endl;
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

    /*void hybrid_cut()
    {
        printf("worker %d begin hybrid cut\n", worker_id);
        vector<omp_lock_t> mirror_lock(related_v_num);
    #pragma omp parallel for
        for (uint64_t i = 0; i < mirror_lock.size(); i++)
            omp_init_lock(&mirror_lock[i]);
            // for (auto &x : mirror_lock)
            //     omp_init_lock(&x);

        printf("worker %d begin hybrid : mirror assign\n", worker_id);

    #pragma omp parallel for schedule(dynamic)
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            Vertex &v = vertex[i];
            int cur_dc = v.current_dc;
            if (v.is_high_degree)
            {
                for (auto &in_neighbour : v.in_edge)
                {
                    int &neighbour_dc = vertex[in_neighbour].current_dc;

                    omp_set_lock(&mirror_lock[in_neighbour]);
                    // vertex[in_neighbour].mirror[neighbour_dc].out_degree++;
                    mirror[neighbour_dc][in_neighbour].out_degree++;
                    // vertex[in_neighbour].mirror[neighbour_dc].use = true;
                    omp_unset_lock(&mirror_lock[in_neighbour]);

                    omp_set_lock(&mirror_lock[i]);
                    // v.mirror[neighbour_dc].in_degree++;
                    mirror[neighbour_dc][i].in_degree++;
                    // v.mirror[neighbour_dc].use = true;
                    omp_unset_lock(&mirror_lock[i]);
                }
            }
            else
            {
                for (auto &in_neighbour : v.in_edge)
                {
                    // printf("%d\n", in_neighbour);

                    int &neighbour_dc = vertex[in_neighbour].current_dc;
                    omp_set_lock(&mirror_lock[in_neighbour]);

                    // vertex[in_neighbour].mirror[cur_dc].out_degree++;
                    mirror[cur_dc][in_neighbour].out_degree++;
                    // vertex[in_neighbour].mirror[cur_dc].use = true;
                    omp_unset_lock(&mirror_lock[in_neighbour]);

                    omp_set_lock(&mirror_lock[i]);
                    // v.mirror[cur_dc].in_degree++;
                    mirror[cur_dc][i].in_degree++;
                    // v.mirror[cur_dc].use = true;
                    omp_unset_lock(&mirror_lock[i]);
                }
            }
        }

        printf("worker %d finished hybrid : mirror assign\n", worker_id);
        // cout << related_v_num << ' ' << agent_in_subgraph.size() << ' ' << DC_num<< endl;

        const int mirror_diff_offset = DC_num * 2 + 1;
        vector<unsigned int> send((uint64_t)(related_v_num)*mirror_diff_offset);


        printf("worker %d begin hybrid : mirror data assign\n", worker_id);
    #pragma omp parallel for schedule(dynamic)
        for (uint64_t i = 0; i < related_v_num; i++)
        {
            Vertex &v = vertex[i];
            uint64_t index = (i)*mirror_diff_offset;
            send[index] = v.id;
            for (int dc = 0; dc < DC_num; dc++)
            {
                // send[index + dc * 2 + 1] = v.mirror[dc].in_degree;
                // send[index + dc * 2 + 2] = v.mirror[dc].out_degree;
                send[index + dc * 2 + 1] = mirror[dc][i].in_degree;
                send[index + dc * 2 + 2] = mirror[dc][i].out_degree;
            }
        }

        printf("worker %d finished hybrid : mirror data assign\n", worker_id);
        vector<unsigned int> recv;
        THB_Allgatherv(send, numprocs, worker_id, recv, false);
        printf("Worker %d received %f GB message, size = %ld, max size = %ld\n", worker_id, kBToGB(getVectorMemoryUsage(recv) / 1024), recv.size(), recv.max_size());

    #pragma omp parallel for schedule(dynamic)
        for (uint64_t i = 0; i < recv.size() / mirror_diff_offset; i++)
        {
            id_type id = recv[i * mirror_diff_offset];
            if (mapped.count(id))
            {
                id_type mapped_id = mapped[id];
                Vertex &v = vertex[mapped_id];
                omp_set_lock(&mirror_lock[mapped_id]);
                for (uint64_t index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                {
                    int in_degree = recv[index + dc * 2];
                    int out_degree = recv[index + dc * 2 + 1];

                    // v.mirror[dc].in_degree += in_degree;
                    // v.mirror[dc].out_degree += out_degree;
                    mirror[dc][mapped_id].in_degree += in_degree;
                    mirror[dc][mapped_id].out_degree += out_degree;
                    // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                    //     v.mirror[dc].use = true;
                }
                omp_unset_lock(&mirror_lock[mapped_id]);
            }
        }
        printf("worker %d finished hybrid cut\n", worker_id);
        MPI_Barrier(MPI_COMM_WORLD);
    }*/

    void hybrid_cut()
    {
        printf("worker %d begin hybrid cut\n", worker_id);
        vector<omp_lock_t> mirror_lock(related_v_num);
#pragma omp parallel for
        for (uint64_t i = 0; i < mirror_lock.size(); i++)
            omp_init_lock(&mirror_lock[i]);
        // for (auto &x : mirror_lock)
        //     omp_init_lock(&x);

        printf("worker %d begin hybrid : mirror assign\n", worker_id);

#pragma omp parallel for schedule(dynamic)
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            Vertex &v = vertex[i];
            int cur_dc = v.current_dc;
            if (v.is_high_degree)
            {
                for (auto &in_neighbour : v.in_edge)
                {
                    int &neighbour_dc = vertex[in_neighbour].current_dc;

                    omp_set_lock(&mirror_lock[in_neighbour]);
                    // vertex[in_neighbour].mirror[neighbour_dc].out_degree++;
                    mirror[neighbour_dc][in_neighbour].out_degree++;
                    // vertex[in_neighbour].mirror[neighbour_dc].use = true;
                    omp_unset_lock(&mirror_lock[in_neighbour]);

                    omp_set_lock(&mirror_lock[i]);
                    // v.mirror[neighbour_dc].in_degree++;
                    mirror[neighbour_dc][i].in_degree++;
                    // v.mirror[neighbour_dc].use = true;
                    omp_unset_lock(&mirror_lock[i]);
                }
            }
            else
            {
                for (auto &in_neighbour : v.in_edge)
                {
                    // printf("%d\n", in_neighbour);

                    int &neighbour_dc = vertex[in_neighbour].current_dc;
                    omp_set_lock(&mirror_lock[in_neighbour]);

                    // vertex[in_neighbour].mirror[cur_dc].out_degree++;
                    mirror[cur_dc][in_neighbour].out_degree++;
                    // vertex[in_neighbour].mirror[cur_dc].use = true;
                    omp_unset_lock(&mirror_lock[in_neighbour]);

                    omp_set_lock(&mirror_lock[i]);
                    // v.mirror[cur_dc].in_degree++;
                    mirror[cur_dc][i].in_degree++;
                    // v.mirror[cur_dc].use = true;
                    omp_unset_lock(&mirror_lock[i]);
                }
            }
        }

        printf("worker %d finished hybrid : mirror assign\n", worker_id);
        // cout << related_v_num << ' ' << agent_in_subgraph.size() << ' ' << DC_num<< endl;

        const int mirror_diff_offset = DC_num * 2 + 1;
        vector<unsigned int> send;

        printf("worker %d begin hybrid : mirror data assign\n", worker_id);

        uint64_t local_send_size = (uint64_t)(related_v_num)*mirror_diff_offset;
        uint64_t total_recv_size;
        MPI_Allreduce(&local_send_size, &total_recv_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);

        const uint64_t MAX_SIZE = (static_cast<uint64_t>(1) << 31); // 2GB
        int n = (total_recv_size + MAX_SIZE) / MAX_SIZE;
        // n = 2;
        // printf("using %d chunks to send %d\n", n, total_recv_size / mirror_diff_offset);

        const uint64_t chunk_size = related_v_num / n; // 计算每个块的大小
        for (uint64_t k = 0; k < n; ++k)
        {
            uint64_t start_index = k * chunk_size;
            uint64_t end_index = (k == n - 1) ? related_v_num : start_index + chunk_size; // 如果是最后一个块，则包含所有剩余的元素
            // printf("from %ld to %ld \n", start_index, end_index);
            send.resize((end_index - start_index) * mirror_diff_offset);

#pragma omp parallel for schedule(dynamic)
            for (uint64_t i = start_index; i < end_index; ++i)
            {
                Vertex &v = vertex[i];
                uint64_t index = (i - start_index) * mirror_diff_offset; // 注意这里的索引是从0开始的，因为我们是在新的块中
                send[index] = v.id;
                for (int dc = 0; dc < DC_num; dc++)
                {
                    send[index + dc * 2 + 1] = mirror[dc][i].in_degree;
                    send[index + dc * 2 + 2] = mirror[dc][i].out_degree;
                }
            }

            // printf("worker %d finished hybrid : mirror data assign for chunk %llu\n", worker_id, k);
            vector<unsigned int> recv;
            THB_Allgatherv(send, numprocs, worker_id, recv, false);
            // printf("Worker %d received %f GB message, size = %ld, max size = %ld\n", worker_id, kBToGB(getVectorMemoryUsage(recv) / 1024), recv.size(), recv.max_size());

#pragma omp parallel for schedule(dynamic)
            for (uint64_t i = 0; i < recv.size() / mirror_diff_offset; ++i)
            {
                id_type id = recv[i * mirror_diff_offset];
                if (mapped.count(id))
                {
                    id_type mapped_id = mapped[id];
                    Vertex &v = vertex[mapped_id];
                    omp_set_lock(&mirror_lock[mapped_id]);
                    for (uint64_t index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                    {
                        int in_degree = recv[index + dc * 2];
                        int out_degree = recv[index + dc * 2 + 1];

                        mirror[dc][mapped_id].in_degree += in_degree;
                        mirror[dc][mapped_id].out_degree += out_degree;
                    }
                    omp_unset_lock(&mirror_lock[mapped_id]);
                }
            }
            // printf("worker %d finished hybrid cut for chunk %llu\n", worker_id, k);
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }
    /*
    void hybrid_cut()
    {
        printf("Begin hybrid cut\n");
        vector<omp_lock_t> mirror_lock(related_v_num);
        for (auto &x : mirror_lock)
            omp_init_lock(&x);

    #pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            Vertex &v = vertex[i];
            int cur_dc = v.current_dc;
            if (v.is_high_degree)
            {
                for (auto &in_neighbour : v.in_edge)
                {
                    int &neighbour_dc = vertex[in_neighbour].current_dc;

                    omp_set_lock(&mirror_lock[in_neighbour]);
                    // vertex[in_neighbour].mirror[neighbour_dc].out_degree++;
                    mirror[neighbour_dc][in_neighbour].out_degree++;
                    // vertex[in_neighbour].mirror[neighbour_dc].use = true;
                    omp_unset_lock(&mirror_lock[in_neighbour]);

                    omp_set_lock(&mirror_lock[i]);
                    // v.mirror[neighbour_dc].in_degree++;
                    mirror[neighbour_dc][i].in_degree++;
                    // v.mirror[neighbour_dc].use = true;
                    omp_unset_lock(&mirror_lock[i]);
                }
            }
            else
            {
                for (auto &in_neighbour : v.in_edge)
                {
                    // printf("%d\n", in_neighbour);

                    int &neighbour_dc = vertex[in_neighbour].current_dc;
                    omp_set_lock(&mirror_lock[in_neighbour]);

                    // vertex[in_neighbour].mirror[cur_dc].out_degree++;
                    mirror[cur_dc][in_neighbour].out_degree++;
                    // vertex[in_neighbour].mirror[cur_dc].use = true;
                    omp_unset_lock(&mirror_lock[in_neighbour]);

                    omp_set_lock(&mirror_lock[i]);
                    // v.mirror[cur_dc].in_degree++;
                    mirror[cur_dc][i].in_degree++;
                    // v.mirror[cur_dc].use = true;
                    omp_unset_lock(&mirror_lock[i]);
                }
            }
        }
        // cout << related_v_num << ' ' << agent_in_subgraph.size() << ' ' << DC_num<< endl;

        const int mirror_diff_offset = DC_num * 2 + 1;
        vector<unsigned int> send((related_v_num)*mirror_diff_offset);

    #pragma omp parallel for
        for (id_type i = 0; i < related_v_num; i++)
        {
            Vertex &v = vertex[i];
            unsigned int index = (i)*mirror_diff_offset;
            send[index] = v.id;
            for (int dc = 0; dc < DC_num; dc++)
            {
                // send[index + dc * 2 + 1] = v.mirror[dc].in_degree;
                // send[index + dc * 2 + 2] = v.mirror[dc].out_degree;
                send[index + dc * 2 + 1] = mirror[dc][i].in_degree;
                send[index + dc * 2 + 2] = mirror[dc][i].out_degree;
            }
        }

        vector<unsigned int> recv;
        THB_Allgatherv(send, numprocs, worker_id, recv, false);
        printf("Worker %d received %f GB message\n", worker_id, kBToGB(getVectorMemoryUsage(recv) / 1024));

    #pragma omp parallel for
        for (id_type i = 0; i < recv.size() / mirror_diff_offset; i++)
        {
            id_type id = recv[i * mirror_diff_offset];
            if (mapped.count(id))
            {
                id_type mapped_id = mapped[id];
                Vertex &v = vertex[mapped_id];
                omp_set_lock(&mirror_lock[mapped_id]);
                for (id_type index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                {
                    int in_degree = recv[index + dc * 2];
                    int out_degree = recv[index + dc * 2 + 1];

                    // v.mirror[dc].in_degree += in_degree;
                    // v.mirror[dc].out_degree += out_degree;
                    mirror[dc][mapped_id].in_degree += in_degree;
                    mirror[dc][mapped_id].out_degree += out_degree;
                    // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                    //     v.mirror[dc].use = true;
                }
                omp_unset_lock(&mirror_lock[mapped_id]);
            }
        }
    }*/
    void calculate_wan()
    {
        for (auto &x : DC)
            x.reset();

        vector<atomic_uint32_t> gather_upload_wan(DC_num);
        vector<atomic_uint32_t> gather_download_wan(DC_num);
        vector<atomic_uint32_t> apply_upload_wan(DC_num);
        vector<atomic_uint32_t> apply_download_wan(DC_num);

#pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            Vertex &v = vertex[i];
            int cur_dc = v.current_dc;
            for (int dc = 0; dc < DC_num; dc++)
            {
                if (dc != cur_dc)
                {
                    // if (v.mirror[dc].in_degree)
                    if (mirror[dc][i].in_degree)
                    {
                        gather_upload_wan[dc]++;
                        gather_download_wan[cur_dc]++;
                    }
                    if (mirror[dc][i].out_degree)
                    {
                        // assert(v.mirror[dc].in_degree == 0);
                        apply_upload_wan[cur_dc]++;
                        apply_download_wan[dc]++;
                        // if (dc == 0)
                        //     printf("------====== %d %d %d\n", v.id, v.current_dc, dc);
                    }
                }
            }
        }

        const int data_offset = 4;
        vector<unsigned int> send_data(data_offset * DC_num);
        for (uint64_t i = 0; i < DC_num; i++)
        {
            send_data[i * data_offset + 0] = gather_upload_wan[i];
            send_data[i * data_offset + 1] = gather_download_wan[i];
            send_data[i * data_offset + 2] = apply_upload_wan[i];
            send_data[i * data_offset + 3] = apply_download_wan[i];
        }

        vector<unsigned int> recv;
        THB_Allgatherv(send_data, numprocs, worker_id, recv, true);

        // printf("Worker %d received %f GB message\n", worker_id, kBToGB(getVectorMemoryUsage(recv) / 1024));
        for (int dc = 0; dc < DC_num; dc++)
        {
            int gu = 0, gd = 0, au = 0, ad = 0;
            for (int n = 0; n < numprocs; n++)
            {
                gu += recv[n * send_data.size() + dc * data_offset + 0];
                gd += recv[n * send_data.size() + dc * data_offset + 1];
                au += recv[n * send_data.size() + dc * data_offset + 2];
                ad += recv[n * send_data.size() + dc * data_offset + 3];
            }
            // DC[dc].gather_upload_wan = gu * DATA_UNIT;
            // DC[dc].gather_download_wan = gd * DATA_UNIT;
            // DC[dc].apply_upload_wan = au * DATA_UNIT;
            // DC[dc].apply_download_wan = ad * DATA_UNIT;

            for (int i = 0; i < gu; i++)
                DC[dc].gather_upload_wan += DATA_UNIT;
            for (int i = 0; i < gd; i++)
                DC[dc].gather_download_wan += DATA_UNIT;
            for (int i = 0; i < au; i++)
                DC[dc].apply_upload_wan += DATA_UNIT;
            for (int i = 0; i < ad; i++)
                DC[dc].apply_download_wan += DATA_UNIT;

            // DC[dc].gather_upload_wan = gu;
            // DC[dc].gather_download_wan = gd;
            // DC[dc].apply_upload_wan = au;
            // DC[dc].apply_download_wan = ad;
        }
    }
    void calculate_time_cost()
    {
        double max_gather_t = 0, max_apply_t = 0;
        double sum_p = 0;
        for (int i = 0; i < DC_num; i++)
        {
            max_gather_t = max(max_gather_t, DC[i].gather_upload_wan / DC[i].UploadBandwidth);
            max_gather_t = max(max_gather_t, DC[i].gather_download_wan / DC[i].DownloadBandwidth);

            max_apply_t = max(max_apply_t, DC[i].apply_upload_wan / DC[i].UploadBandwidth);
            max_apply_t = max(max_apply_t, DC[i].apply_download_wan / DC[i].DownloadBandwidth);

            sum_p += DC[i].UploadPrice * (DC[i].apply_upload_wan + DC[i].gather_upload_wan);
        }
        transfer_time = max_apply_t + max_gather_t;
        transfer_cost = sum_p;
    }
    void calculate_network_time_price(double &t, double &p, vector<DataCenter> &DC_)
    {
        double max_gather_t = 0, max_apply_t = 0;
        double sum_p = 0;
        for (int i = 0; i < DC_num; i++)
        {
            max_gather_t = max(max_gather_t, DC_[i].gather_upload_wan / DC_[i].UploadBandwidth);
            max_gather_t = max(max_gather_t, DC_[i].gather_download_wan / DC_[i].DownloadBandwidth);

            max_apply_t = max(max_apply_t, DC_[i].apply_upload_wan / DC_[i].UploadBandwidth);
            max_apply_t = max(max_apply_t, DC_[i].apply_download_wan / DC_[i].DownloadBandwidth);

            sum_p += DC_[i].UploadPrice * (DC_[i].apply_upload_wan + DC_[i].gather_upload_wan);
        }
        t = max_apply_t + max_gather_t;
        p = sum_p;
    }
    int sum = 0;
    void moveVertex2(id_type v, int dc)
    {
        if (vertex[v].current_dc == dc)
            return;

        int cur_dc = vertex[v].current_dc;
        int init_dc = vertex[v].init_dc;
        if (cur_dc == init_dc)
            movecost += MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
        else if (dc == init_dc)
            movecost -= MOVE_DATA_UNIT * DC[init_dc].UploadPrice;

        if (vertex[v].is_high_degree)
        {
            for (int i = 0; i < DC_num; i++)
            {
                if (i == cur_dc)
                    continue;
                if (mirror[i][v].in_degree > 0)
                    DC[cur_dc].gather_download_wan -= DATA_UNIT;
                if (mirror[i][v].out_degree > 0)
                    DC[cur_dc].apply_upload_wan -= DATA_UNIT;
            }

            if (mirror[cur_dc][v].in_degree > 0)
                DC[cur_dc].gather_upload_wan += DATA_UNIT;
            if (mirror[cur_dc][v].out_degree > 0)
                DC[cur_dc].gather_download_wan += DATA_UNIT;

            for (int i = 0; i < DC_num; i++)
            {
                if (i == cur_dc)
                    continue;
                if (mirror[i][v].in_degree > 0)
                    DC[dc].gather_download_wan += DATA_UNIT;
                if (mirror[i][v].out_degree > 0)
                    DC[dc].apply_upload_wan += DATA_UNIT;
            }
            for (id_type &out_neighbour : *vertex[v].out_edge)
            {
                id_type vvv = out_neighbour;
                mirror[cur_dc][out_neighbour].in_degree++;
                mirror[dc][out_neighbour].in_degree--;
                // vertex[vvv].mirror_delta[cur_dc].in_degree--;
                // vertex[out_neighbour].mirror[dc].in_degree++;
                // int a = mirror[cur_dc][out_neighbour].in_degree + mirror[dc][out_neighbour].in_degree;
                // mirror[dc][out_neighbour].add(-1, 0);
                // sum += --mirror[out_neighbour][cur_dc].in_degree + mirror[out_neighbour][dc].in_degree;
                sum++;
            }
        }
    }
    void moveVertex(id_type v, int dc) // 移动顶点v到服务器dc
    {
        if (vertex[v].current_dc == dc)
            return;

        // if(!vertex[v].is_high_degree)   return;

        // change_tag[v] = true;
        int origin_dc = vertex[v].current_dc;
        int init_dc = vertex[v].init_dc;
        if (origin_dc == init_dc) // 移出初始dc，需要产生消耗
        {
            movecost += MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
        }
        else if (dc == init_dc) // 移回初始dc，取消消耗
        {
            movecost -= MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
        }

        if (vertex[v].is_high_degree) // 移动的顶点是High Degree的
        {
            // 取消所有mirror的带来的消耗
            for (int i = 0; i < DC_num; i++)
            {
                if (i == origin_dc)
                    continue;
                if (mirror[i][v].in_degree > 0)
                    DC[origin_dc].gather_download_wan -= DATA_UNIT;
                if (mirror[i][v].out_degree > 0)
                    DC[origin_dc].apply_upload_wan -= DATA_UNIT;
            }
            // 判断是否需要保留一个mirror

            if (mirror[origin_dc][v].in_degree > 0)
                DC[origin_dc].gather_upload_wan += DATA_UNIT;
            if (mirror[origin_dc][v].out_degree > 0)
                DC[origin_dc].apply_download_wan += DATA_UNIT;

            // 判断服务器dc是否已经有mirror

            if (mirror[dc][v].in_degree > 0)
                DC[dc].gather_upload_wan -= DATA_UNIT;
            if (mirror[dc][v].out_degree > 0)
                DC[dc].apply_download_wan -= DATA_UNIT;

            // 无论是否有镜像，先继承mirror已有的本地关系

            // 更新mirror带来的消耗
            for (int i = 0; i < DC_num; i++)
            {
                if (i == dc)
                    continue;
                if (mirror[i][v].in_degree > 0)
                    DC[dc].gather_download_wan += DATA_UNIT;
                if (mirror[i][v].out_degree > 0)
                    DC[dc].apply_upload_wan += DATA_UNIT;
            }

            // 迁移出边
            for (id_type &out_neighbour : *vertex[v].out_edge)
            {
                // 移动出边为High Degree的邻顶
                if (vertex[out_neighbour].is_high_degree)
                {
                    if (out_neighbour >= agent_in_subgraph_vector.size())
                        change_tag[out_neighbour] = true;
                    mirror[dc][v].out_degree++;

                    // vertex[v].mirror_delta[dc].out_degree++;
                    mirror[origin_dc][v].out_degree--;
                    // vertex[v].mirror_delta[origin_dc].out_degree--;

                    if (mirror[origin_dc][v].out_degree == 0)
                    {
                        DC[origin_dc].apply_download_wan -= DATA_UNIT;
                        DC[dc].apply_upload_wan -= DATA_UNIT;
                    }

                    // printf("%d %d\n", mirror[origin_dc][out_neighbour].in_degree, mirror[dc][out_neighbour].in_degree);
                    // 拔

                    // if(mirror[origin_dc][out_neighbour].in_degree && mirror[dc][out_neighbour].in_degree)
                    //     xxx++;
                    --mirror[origin_dc][out_neighbour].in_degree;

                    // if(mirror[origin_dc][out_neighbour].in_degree && mirror[dc][out_neighbour].in_degree)
                    //     continue;

                    if (out_neighbour >= agent_in_subgraph_vector.size())
                        mirror_delta[origin_dc][out_neighbour].in_degree--;
                    int out_neighbour_dc = vertex[out_neighbour].current_dc;

                    if (mirror[origin_dc][out_neighbour].in_degree == 0 &&
                        out_neighbour_dc != origin_dc)
                    {
                        DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                        DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
                    }
                    // 放
                    ++mirror[dc][out_neighbour].in_degree;
                    if (out_neighbour >= agent_in_subgraph_vector.size())
                        mirror_delta[dc][out_neighbour].in_degree++;

                    if (out_neighbour_dc != dc && mirror[dc][out_neighbour].in_degree == 1)
                    {
                        DC[dc].gather_upload_wan += DATA_UNIT;
                        DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
                    }
                }
            }
        }
        else // 移动的顶点是Low Degree的
        {
            // 首先移出mirror带来的消耗
            for (int i = 0; i < DC_num; i++)
            {
                if (i == origin_dc)
                    continue;
                if (mirror[i][v].out_degree > 0)
                    DC[origin_dc].apply_upload_wan -= DATA_UNIT;
            }
            // 判断是否需要保留mirror

            if (mirror[origin_dc][v].in_degree > 0)
                DC[origin_dc].gather_upload_wan += DATA_UNIT,
                    DC[dc].gather_download_wan += DATA_UNIT; // 需要临时加上
            if (mirror[origin_dc][v].out_degree > 0)
                DC[origin_dc].apply_download_wan += DATA_UNIT;

            // 判断是否已有mirror
            if (mirror[dc][v].in_degree > 0)
                DC[dc].gather_upload_wan -= DATA_UNIT;
            if (mirror[dc][v].out_degree > 0)
                DC[dc].apply_download_wan -= DATA_UNIT;

            // 继承mirror的关系

            // 恢复mirror的消耗
            for (int i = 0; i < DC_num; i++)
            {
                if (i == dc)
                    continue;
                if (mirror[i][v].out_degree > 0)
                    DC[dc].apply_upload_wan += DATA_UNIT;
            }

            // 移动入边
            for (auto in_neighbour : vertex[v].in_edge)
            {
                if (in_neighbour >= agent_in_subgraph_vector.size())
                    change_tag[in_neighbour] = true;
                mirror[origin_dc][v].in_degree--;
                // vertex[v].mirror_delta[origin_dc].in_degree--;
                mirror[dc][v].in_degree++;
                // vertex[v].mirror_delta[dc].in_degree++;

                if (mirror[origin_dc][v].in_degree == 0)
                {
                    DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                    DC[dc].gather_download_wan -= DATA_UNIT;
                }
                // if (vertex[in_neighbour].is_high_degree)

                int in_neighbour_dc = vertex[in_neighbour].current_dc;
                mirror[origin_dc][in_neighbour].out_degree--;
                if (in_neighbour >= agent_in_subgraph_vector.size())
                    mirror_delta[origin_dc][in_neighbour].out_degree--;

                if (in_neighbour_dc != origin_dc &&
                    mirror[origin_dc][in_neighbour].out_degree == 0)
                {
                    DC[origin_dc].apply_download_wan -= DATA_UNIT;
                    DC[in_neighbour_dc].apply_upload_wan -= DATA_UNIT;
                }

                mirror[dc][in_neighbour].out_degree++;
                if (in_neighbour >= agent_in_subgraph_vector.size())
                    mirror_delta[dc][in_neighbour].out_degree++;
                if (in_neighbour_dc != dc &&
                    mirror[dc][in_neighbour].out_degree == 1)
                {
                    DC[dc].apply_download_wan += DATA_UNIT;
                    DC[in_neighbour_dc].apply_upload_wan += DATA_UNIT;
                }
            }

            // 移动High Degree的出边
            for (auto out_neighbour : *vertex[v].out_edge)
            {
                if (vertex[out_neighbour].is_high_degree)
                {
                    if (out_neighbour >= agent_in_subgraph_vector.size())
                        change_tag[out_neighbour] = true;
                    mirror[dc][v].out_degree++;
                    // vertex[v].mirror_delta[dc].out_degree++;
                    mirror[origin_dc][v].out_degree--;
                    // vertex[v].mirror_delta[origin_dc].out_degree--;

                    if (mirror[origin_dc][v].out_degree == 0)
                    {
                        DC[origin_dc].apply_download_wan -= DATA_UNIT;
                        DC[dc].apply_upload_wan -= DATA_UNIT;
                        if (mirror[origin_dc][v].in_degree == 0)
                            ;
                    }

                    // 拔
                    mirror[origin_dc][out_neighbour].in_degree--;
                    if (out_neighbour >= agent_in_subgraph_vector.size())
                        mirror_delta[origin_dc][out_neighbour].in_degree--;
                    int out_neighbour_dc = vertex[out_neighbour].current_dc;
                    if (mirror[origin_dc][out_neighbour].in_degree == 0 &&
                        out_neighbour_dc != origin_dc)
                    {
                        DC[origin_dc].gather_upload_wan -= DATA_UNIT;
                        DC[out_neighbour_dc].gather_download_wan -= DATA_UNIT;
                    }
                    // 放
                    mirror[dc][out_neighbour].in_degree++;
                    if (out_neighbour >= agent_in_subgraph_vector.size())
                        mirror_delta[dc][out_neighbour].in_degree++;
                    if (out_neighbour_dc != dc && mirror[dc][out_neighbour].in_degree == 1)
                    {
                        DC[dc].gather_upload_wan += DATA_UNIT;
                        DC[out_neighbour_dc].gather_download_wan += DATA_UNIT;
                    }
                }
            }
        }
        vertex[v].current_dc = dc;

        calculate_time_cost();
    }

    /*
     void moveVertex2(id_type v, int dc)
     {
         if(vertex[v].current_dc == dc)  return;

         int cur_dc = vertex[v].current_dc;
         int init_dc = vertex[v].init_dc;
         if(cur_dc == init_dc)   movecost += MOVE_DATA_UNIT * DC[init_dc].UploadPrice;
         else if(dc == init_dc)  movecost -= MOVE_DATA_UNIT * DC[init_dc].UploadPrice;


         if(vertex[v].is_high_degree)
         {
             for(int i = 0; i < DC_num; i++)
             {
                 if(i == cur_dc) continue;
                 if(mirror[i][v].in_degree > 0)
                     DC[cur_dc].gather_download_wan -= DATA_UNIT;
                 if(mirror[i][v].out_degree > 0)
                     DC[cur_dc].apply_upload_wan -= DATA_UNIT;
             }

             if(mirror[cur_dc][v].in_degree > 0)
                 DC[cur_dc].gather_upload_wan += DATA_UNIT;
             if(mirror[cur_dc][v].out_degree > 0)
                 DC[cur_dc].gather_download_wan += DATA_UNIT;

             for(int i = 0; i < DC_num; i++)
             {
                 if(i == cur_dc) continue;
                 if(mirror[i][v].in_degree > 0)
                     DC[dc].gather_download_wan += DATA_UNIT;
                 if(mirror[i][v].out_degree > 0)
                     DC[dc].apply_upload_wan += DATA_UNIT;
             }
             for(id_type &out_neighbour : vertex[v].out_edge)
             {
                 id_type vvv = out_neighbour;
                 // vertex[vvv].mirror_delta[cur_dc].in_degree--;
                 // mirror[dc][out_neighbour].in_degree++;
                 mirror[cur_dc][out_neighbour].in_degree--;
                 mirror[dc][out_neighbour].in_degree++;
                 // sum += mirror[out_neighbour][cur_dc].in_degree + mirror[out_neighbour][dc].in_degree;
             }
         }
     }
     */
    void local_move_test()
    {
        for (id_type v = 0; v < agent_in_subgraph_vector.size(); v++)
        {
            // if(vertex[v].i`s_high_degree)
            moveVertex(v, 0);
        }
        print_in_order();
        for (id_type v = 0; v < agent_in_subgraph_vector.size(); v++)
        {
            moveVertex(v, vertex[v].init_dc);
        }
        print_in_order();
    }
    void print_top_1K_mirror_info()
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (worker_id == 0)
            for (int i = 0; i < 1000; i++)
            {
                // MPI_Barrier(MPI_COMM_WORLD);
                if (!agent_in_subgraph.count(i))
                    continue;
                printf("%d %s in DC %d --> ", i, vertex[mapped[i]].is_high_degree ? "HDN" : "LDN", vertex[mapped[i]].current_dc);
                for (int dc = 0; dc < DC_num; dc++)
                {
                    printf("%d-%d \t\t", mirror[dc][mapped[i]].in_degree, mirror[dc][mapped[i]].out_degree);
                }
                printf("\n");
            }
    }
    void update_global_env(const unordered_set<id_type> &s)
    {
        vector<unsigned int> local_change_v;
        local_change_v.assign(s.begin(), s.end());
        update_global_env(local_change_v);
    }
    /*
    void update_global_env(const vector<id_type> &local_change_v)
    {
        // update_global_change()

        // 更新local agent变化的DC
        apply_subgraph_agent_DC(update_global_change(local_change_v), COUPLE_SIZE);

        const int mirror_diff_offset = DC_num * 2 + 1;
        vector<vector<unsigned int>> send(numprocs);

        vector<omp_lock_t> write_lock(numprocs);
        for (auto &x : write_lock)
            omp_init_lock(&x);

            // 发送1-hop的mirror变化值
    #pragma omp parallel for
        for (id_type i = agent_in_subgraph_vector.size(); i < related_v_num; i++)
        {
            if (!change_tag[i])
                continue;
            change_tag[i] = false;
            Vertex &v = vertex[i];
            int worker = v.init_dc;
            omp_set_lock(&write_lock[worker]);
            send[worker].push_back(v.id);
            for (int dc = 0; dc < DC_num; dc++)
            {
                send[worker].push_back(mirror_delta[dc][i].in_degree);
                send[worker].push_back(mirror_delta[dc][i].out_degree);
                mirror_delta[dc][i].in_degree = mirror_delta[dc][i].out_degree = 0;
            }
            omp_unset_lock(&write_lock[worker]);
        }

        // vector<int> send_count(numprocs);
        // for (int i = 0; i < numprocs; i++)
        //     send_count[i] = send[i].size();

        // vector<unsigned int> oneD_vector;
        // for (auto &v : send)
        // {
        //     oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
        // }

        // 合并成1维并发送
        vector<unsigned int> recv = move(THB_Alltoallv(send, numprocs, worker_id));

        vector<omp_lock_t> mirror_lock(related_v_num);
        for (auto &x : mirror_lock)
            omp_init_lock(&x);

    #pragma omp parallel for
        for (id_type i = 0; i < recv.size() / mirror_diff_offset; i++)
        {
            id_type id = recv[i * mirror_diff_offset];
            // if (agent_in_subgraph.count(id))
            {
                id_type mapped_id = mapped[id];
                Vertex &v = vertex[mapped_id];
                omp_set_lock(&mirror_lock[mapped_id]);
                change_tag[mapped_id] = true;
                for (id_type index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                {
                    unsigned int in_degree = recv[index + dc * 2];
                    unsigned int out_degree = recv[index + dc * 2 + 1];

                    mirror[dc][mapped_id].in_degree += in_degree;
                    mirror[dc][mapped_id].out_degree += out_degree;
                    // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                    //     v.mirror[dc].use = true;
                }
                omp_unset_lock(&mirror_lock[mapped_id]);
            }
        }

        // send.resize(agent_in_subgraph_vector.size() * mirror_diff_offset);
        for (auto &x : send)
            x.clear();

    #pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            if (change_tag[i])
            {
                change_tag[i] = false;
                Vertex &v = vertex[i];
                unsigned int index = (i)*mirror_diff_offset;
                for (auto &j : v.worker_has_1hop)
                {
                    omp_set_lock(&write_lock[j]);
                    send[j].push_back(v.id);
                    for (int dc = 0; dc < DC_num; dc++)
                    {
                        send[j].push_back(mirror[dc][i].in_degree);
                        send[j].push_back(mirror[dc][i].out_degree);
                    }
                    omp_unset_lock(&write_lock[j]);
                }
            }
        }

        // for (int i = 0; i < numprocs; i++)
        //     send_count[i] = send[i].size();

        // oneD_vector.clear();
        // for (auto &v : send)
        // {
        //     oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
        // }

        recv = move(THB_Alltoallv(send, numprocs, worker_id));
        // printf("Worker %d received %f GB message\n", worker_id, kBToGB(getVectorMemoryUsage(recv) / 1024));

        // 同步1-hop mirror信息
    #pragma omp parallel for
        for (id_type i = 0; i < recv.size() / mirror_diff_offset; i++)
        {
            id_type id = recv[i * mirror_diff_offset];
            // if (mapped.count(id))
            {
                id_type mapped_id = mapped[id];
                Vertex &v = vertex[mapped_id];
                // omp_set_lock(&mirror_lock[mapped_id]);
                for (id_type index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                {
                    int in_degree = recv[index + dc * 2];
                    int out_degree = recv[index + dc * 2 + 1];

                    mirror[dc][mapped_id].in_degree = in_degree;
                    mirror[dc][mapped_id].out_degree = out_degree;
                    // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                    //     v.mirror[dc].use = true;
                }
                // omp_unset_lock(&mirror_lock[mapped_id]);
            }
        }

        calculate_wan();
        calculate_time_cost();
    }*/

    void update_global_env(const vector<id_type> &local_change_v)
    {
        // update_global_change()

        // 更新local agent变化的DC
        apply_subgraph_agent_DC(update_global_change(local_change_v), COUPLE_SIZE);

        const int mirror_diff_offset = DC_num * 2 + 1;
        const uint64_t MAX_SIZE = static_cast<uint64_t>(1) << 30; // 2GB

        uint64_t local_send_size = related_v_num;
        uint64_t total_recv_size;

        MPI_Allreduce(&local_send_size, &total_recv_size, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);

        int n = (total_recv_size + MAX_SIZE) / MAX_SIZE;
        // printf("Update global env : n = %d\n", n);

        const int chunk_size = related_v_num / n; // 计算每个块的大小

        vector<vector<unsigned int>> send(numprocs);

        vector<omp_lock_t> write_lock(numprocs);
        for (auto &x : write_lock)
            omp_init_lock(&x);

        for (int k = 0; k < n; ++k)
        {
            uint64_t start_index = k * chunk_size;
            uint64_t end_index = (k == n - 1) ? related_v_num : start_index + chunk_size; // 如果是最后一个块，则包含所有剩余的元素

// 发送1-hop的mirror变化值
#pragma omp parallel for
            for (uint64_t i = start_index; i < end_index; i++) // 注意这里的循环范围是当前块的范围
            {
                // ... (其他代码保持不变)
                if (!change_tag[i])
                    continue;
                change_tag[i] = false;
                Vertex &v = vertex[i];
                int worker = v.init_dc;
                omp_set_lock(&write_lock[worker]);
                send[worker].push_back(v.id);
                for (int dc = 0; dc < DC_num; dc++)
                {
                    send[worker].push_back(mirror_delta[dc][i].in_degree);
                    send[worker].push_back(mirror_delta[dc][i].out_degree);
                    mirror_delta[dc][i].in_degree = mirror_delta[dc][i].out_degree = 0;
                }
                omp_unset_lock(&write_lock[worker]);
            }

            // ... (发送和接收代码保持不变)

            vector<unsigned int> recv = move(THB_Alltoallv(send, numprocs, worker_id));

            vector<omp_lock_t> mirror_lock(related_v_num);
            for (auto &x : mirror_lock)
                omp_init_lock(&x);

#pragma omp parallel for
            for (id_type i = 0; i < recv.size() / mirror_diff_offset; i++)
            {
                id_type id = recv[i * mirror_diff_offset];
                // if (agent_in_subgraph.count(id))
                {
                    id_type mapped_id = mapped[id];
                    Vertex &v = vertex[mapped_id];
                    omp_set_lock(&mirror_lock[mapped_id]);
                    change_tag[mapped_id] = true;
                    for (id_type index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                    {
                        unsigned int in_degree = recv[index + dc * 2];
                        unsigned int out_degree = recv[index + dc * 2 + 1];

                        mirror[dc][mapped_id].in_degree += in_degree;
                        mirror[dc][mapped_id].out_degree += out_degree;
                        // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                        //     v.mirror[dc].use = true;
                    }
                    omp_unset_lock(&mirror_lock[mapped_id]);
                }
            }

            // send.resize(agent_in_subgraph_vector.size() * mirror_diff_offset);
            for (auto &x : send)
                x.clear();

#pragma omp parallel for
            for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
            {
                if (change_tag[i])
                {
                    change_tag[i] = false;
                    Vertex &v = vertex[i];
                    unsigned int index = (i)*mirror_diff_offset;
                    for (auto &j : v.worker_has_1hop)
                    {
                        omp_set_lock(&write_lock[j]);
                        send[j].push_back(v.id);
                        for (int dc = 0; dc < DC_num; dc++)
                        {
                            send[j].push_back(mirror[dc][i].in_degree);
                            send[j].push_back(mirror[dc][i].out_degree);
                        }
                        omp_unset_lock(&write_lock[j]);
                    }
                }
            }

            // for (int i = 0; i < numprocs; i++)
            //     send_count[i] = send[i].size();

            // oneD_vector.clear();
            // for (auto &v : send)
            // {
            //     oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
            // }

            recv = move(THB_Alltoallv(send, numprocs, worker_id));
            // printf("Worker %d received %f GB message\n", worker_id, kBToGB(getVectorMemoryUsage(recv) / 1024));

            // 同步1-hop mirror信息
#pragma omp parallel for
            for (id_type i = 0; i < recv.size() / mirror_diff_offset; i++)
            {
                id_type id = recv[i * mirror_diff_offset];
                // if (mapped.count(id))
                {
                    id_type mapped_id = mapped[id];
                    Vertex &v = vertex[mapped_id];
                    // omp_set_lock(&mirror_lock[mapped_id]);
                    for (id_type index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                    {
                        int in_degree = recv[index + dc * 2];
                        int out_degree = recv[index + dc * 2 + 1];

                        mirror[dc][mapped_id].in_degree = in_degree;
                        mirror[dc][mapped_id].out_degree = out_degree;
                        // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                        //     v.mirror[dc].use = true;
                    }
                    // omp_unset_lock(&mirror_lock[mapped_id]);
                }
            }

            // 清理当前块的send向量
            for (auto &x : send)
                x.clear();
        }

        // vector<int> send_count(numprocs);
        // for (int i = 0; i < numprocs; i++)
        //     send_count[i] = send[i].size();

        // vector<unsigned int> oneD_vector;
        // for (auto &v : send)
        // {
        //     oneD_vector.insert(oneD_vector.end(), v.begin(), v.end());
        // }

        // 合并成1维并发送

        calculate_wan();
        calculate_time_cost();
    }
    /*
    void update_global_env()
    {
        // update_global_change()

        omp_lock_t write_lock;
        omp_init_lock(&write_lock);

        vector<unsigned int> local_change_v;

    #pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            Vertex &v = vertex[i];
            if (change_tag[i])
            {
                change_tag[i] = false;
                omp_set_lock(&write_lock);
                local_change_v.emplace_back(v.id);
                omp_unset_lock(&write_lock);
            }
        }
        apply_subgraph_agent_DC(update_global_change(local_change_v), COUPLE_SIZE, false);

        const int mirror_diff_offset = DC_num * 2 + 1;
        vector<unsigned int> send;

    #pragma omp parallel for
        for (id_type i = agent_in_subgraph_vector.size(); i < related_v_num; i++)
        {
            if (!change_tag[i])
                continue;
            change_tag[i] = false;
            Vertex &v = vertex[i];
            omp_set_lock(&write_lock);
            send.emplace_back(v.id);
            for (int dc = 0; dc < DC_num; dc++)
            {
                send.emplace_back(v.mirror_delta[dc].in_degree);
                send.emplace_back(v.mirror_delta[dc].out_degree);
                v.mirror_delta[dc].in_degree = v.mirror_delta[dc].out_degree = 0;
            }
            omp_unset_lock(&write_lock);
        }

        vector<unsigned int> recv = move(THB_Allgatherv(send, numprocs, worker_id));

        vector<omp_lock_t> mirror_lock(related_v_num);
        for (auto &x : mirror_lock)
            omp_init_lock(&x);

    #pragma omp parallel for
        for (id_type i = 0; i < recv.size() / mirror_diff_offset; i++)
        {
            id_type id = recv[i * mirror_diff_offset];
            if (agent_in_subgraph.count(id))
            {
                id_type mapped_id = mapped[id];
                Vertex &v = vertex[mapped_id];
                omp_set_lock(&mirror_lock[mapped_id]);
                for (id_type index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                {
                    unsigned int in_degree = recv[index + dc * 2];
                    unsigned int out_degree = recv[index + dc * 2 + 1];

                    v.mirror[dc].in_degree += in_degree;
                    v.mirror[dc].out_degree += out_degree;
                    // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                    //     v.mirror[dc].use = true;
                }
                omp_unset_lock(&mirror_lock[mapped_id]);
            }
        }

        send.resize(agent_in_subgraph_vector.size() * mirror_diff_offset);

    #pragma omp parallel for
        for (id_type i = 0; i < agent_in_subgraph_vector.size(); i++)
        {
            Vertex &v = vertex[i];
            unsigned int index = (i)*mirror_diff_offset;
            send[index] = v.id;
            for (int dc = 0; dc < DC_num; dc++)
            {
                send[index + dc * 2 + 1] = v.mirror[dc].in_degree;
                send[index + dc * 2 + 2] = v.mirror[dc].out_degree;
            }
        }

        recv = move(THB_Allgatherv(send, numprocs, worker_id));
        printf("Worker %d received %f GB message\n", worker_id, kBToGB(getVectorMemoryUsage(recv) / 1024));

    #pragma omp parallel for
        for (id_type i = 0; i < recv.size() / mirror_diff_offset; i++)
        {
            id_type id = recv[i * mirror_diff_offset];
            if (mapped.count(id))
            {
                id_type mapped_id = mapped[id];
                Vertex &v = vertex[mapped_id];
                omp_set_lock(&mirror_lock[mapped_id]);
                for (id_type index = i * mirror_diff_offset + 1, dc = 0; dc < DC_num; dc++)
                {
                    int in_degree = recv[index + dc * 2];
                    int out_degree = recv[index + dc * 2 + 1];

                    v.mirror[dc].in_degree = in_degree;
                    v.mirror[dc].out_degree = out_degree;
                    // if (v.mirror[dc].in_degree || v.mirror[dc].out_degree)
                    //     v.mirror[dc].use = true;
                }
                omp_unset_lock(&mirror_lock[mapped_id]);
            }
        }

        calculate_wan();
        calculate_time_cost();
    }*/
    void global_move_test()
    {
        print_in_order();
        vector<unsigned int> tmp(agent_in_subgraph_vector.size());
        iota(tmp.begin(), tmp.end(), 0);
        int count = vertex_num / DC_num;
        for (id_type v = 0; v < agent_in_subgraph_vector.size(); v++)
        {
            // if(vertex[v].i`s_high_degree)
            int dc = 0;
            moveVertex(v, dc);
        }
        update_global_env(tmp);
        print_in_order();
        for (id_type v = 0; v < agent_in_subgraph_vector.size(); v++)
        {
            moveVertex(v, vertex[v].init_dc);
        }
        update_global_env(tmp);
        print_in_order();
    }
};

#endif