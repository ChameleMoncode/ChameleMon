#ifndef _EMALGORITHM_FCM_H_
#define _EMALGORITHM_FCM_H_

#include <vector>
#include <cstdint>
#include <cmath>
#include <unordered_map>
#include <iostream>
#include <functional>
#include <thread>
#include <unistd.h>

using std::vector;
using std::unordered_map;

template<int DEPTH, int W0, uint32_t THRESL1, uint32_t THRESL2>
class EM_FCM
{
    vector<vector<vector<uint32_t> > > newsk;
    vector<vector<vector<vector<vector<uint32_t> > > > > newsk_thres;
    vector<vector<int> > m;
    vector<vector<vector<uint32_t> > > counter_dist;
    int max_val;

public:
    vector<double> ns;
    vector<double> dist_old, dist_new;
    int iter = 0;

private:
    double n_old = 0; // old cardinality
    double n_new = 0; // new cardinality

    struct BetaGenerator // for a single degree (same as MRAC's EM)
    {
        int sum;
        int now_flow_num;
        int flow_num_limit;
        vector<int> now_result;

        explicit BetaGenerator(int _sum): sum(_sum)
        {
            now_flow_num = 0; // initialize

            if (sum > 600) { // 1000 for large data, 600 for small data
                flow_num_limit = 2;
            }
            else if (sum > 250) // 500 for large data, 250 for small data
                flow_num_limit = 3;
            else if (sum > 100)
                flow_num_limit = 4;
            else if (sum > 50)
                flow_num_limit = 5;
            else
                flow_num_limit = 6;
        }

        bool get_new_comb()
        {
            for (int j = now_flow_num - 2; j >= 0; --j) {
                int t = ++now_result[j];
                for (int k = j + 1; k < now_flow_num - 1; ++k) {
                    now_result[k] = t;
                }
                int partial_sum = 0;
                for (int k = 0; k < now_flow_num - 1; ++k) {
                    partial_sum += now_result[k];
                }
                int remain = sum - partial_sum;
                if (remain >= now_result[now_flow_num - 2]) {
                    now_result[now_flow_num - 1] = remain;
                    return true;
                }
            }
            return false;
        }

        bool get_next()
        {
            while (now_flow_num <= flow_num_limit) {
                switch (now_flow_num) {
                case 0:
                    now_flow_num = 1;
                    now_result.resize(1);
                    now_result[0] = sum;
                    return true;
                case 1:
                    now_flow_num = 2;
                    now_result[0] = 0;
                    // fallthrough
                default:
                    now_result.resize(now_flow_num);
                    if (get_new_comb()) {
                        return true;
                    } else {
                        now_flow_num++;
                        for (int i = 0; i < now_flow_num - 2; ++i) {
                            now_result[i] = 1;
                        }
                        now_result[now_flow_num - 2] = 0;
                    }
                }
            }
            return false;
        }
    };


    double get_p_from_beta(BetaGenerator & bt, double lambda, vector<double> & now_dist, double now_n, int d)
    {
        unordered_map<uint32_t, uint32_t> mp;
        for (int i = 0; i < bt.now_flow_num; ++i) {
            mp[bt.now_result[i]]++;
        }
        double ret = std::exp(-lambda);
        for (auto & kv: mp) {
            uint32_t fi = kv.second;
            uint32_t si = kv.first;
            double lambda_i = now_n * now_dist[si] / W0;
            ret *= (std::pow(lambda_i, fi)) / factorial(fi);
        }
        return ret;
    }

    struct BetaGenerator_highdeg // for degree > 1, specifically for FCM-Sketch
    {
        int sum; // value
        int beta_degree; // degree
        vector<vector<uint32_t> > thres; // list of <. . .>
        int simplified;
        int now_flow_num;
        int flow_num_limit;
        vector<int> now_result;


        explicit BetaGenerator_highdeg(int _sum, vector<vector<uint32_t> > _thres, int _degree): sum(_sum), beta_degree(_degree)
        {
            // must initialize now_flow_num as 0
            now_flow_num = 0;
            simplified = 0;
            thres = _thres; // interpretation of threshold

            /**** For large dataset and cutoff ****/
            if (sum > 1100) 
                flow_num_limit = beta_degree;
            else if (sum > 550) 
                flow_num_limit = std::max(beta_degree, 3);
            else
                flow_num_limit = std::max(beta_degree, 4);

            // Note that the counters of degree D (> 1) includes at least D flows. 
            // But, the combinatorial complexity also increases to O(N^D), which takes a lot of time.
            // To truncate the compexity, we introduce a simplified version that prohibits the number of flows less than D.
            // As explained in the paper, its effect is quite limited as the number of such counters are only a few.

            // "simplified" keeps the original beta_degree
            // 15k, 15k, 7k
            if (beta_degree >= 4 and sum < 10000){
                simplified = beta_degree;
                flow_num_limit = 3;
                beta_degree = 3;
            }
            else if (beta_degree >= 4 and sum >= 10000){
                simplified = beta_degree;
                flow_num_limit = 2;
                beta_degree = 2;
            }
            else if (beta_degree == 3 and sum > 5000){
                simplified = beta_degree;
                flow_num_limit = 2;
                beta_degree = 2;
            }
        }


        bool get_new_comb()
        {
            // input: now, always flow num >= 2..
            for (int j = now_flow_num - 2; j >= 0; --j) {
                int t = ++now_result[j];
                for (int k = j + 1; k < now_flow_num - 1; ++k) {
                    now_result[k] = t;
                }
                int partial_sum = 0;
                for (int k = 0; k < now_flow_num - 1; ++k) {
                    partial_sum += now_result[k];
                }
                int remain = sum - partial_sum;
                if (remain >= now_result[now_flow_num - 2]) {
                    now_result[now_flow_num - 1] = remain;
                    return true;
                }
            }
            return false;
        }


        bool get_next()
        {
            // Need to test it is available combination or not, based on newsk_thres (bayesian)
            while (now_flow_num <= flow_num_limit) {
                switch (now_flow_num) {
                case 0:
                    now_flow_num = 1;
                    now_result.resize(1);
                    now_result[0] = sum;
                    /*****************************/
                    if (beta_degree == 1) // will never be occured
                        return true;
                    /*****************************/
                case 1:
                    now_flow_num = 2;
                    now_result[0] = 0;
                default: // fall through
                    now_result.resize(now_flow_num);
                    if (get_new_comb()) {
                        /******* condition_check *******/
                        if (simplified == 0){
                            if (condition_check_fcm())
                                return true;
                        }
                        else if (simplified == 3){
                            if (condition_check_fcm_simple2())
                                return true;
                        }
                        else if (beta_degree == 3) {
                            if (condition_check_fcm_simple4to3())
                                return true;
                        }
                        else if (beta_degree == 2) {
                            if (condition_check_fcm_simple4to2())
                                return true;
                        }
                        /*****************************/
                    } else { // no more combination -> go to next flow number
                        now_flow_num++;
                        for (int i = 0; i < now_flow_num - 2; ++i) {
                            now_result[i] = 1;
                        }
                        now_result[now_flow_num - 2] = 0;
                    }
                }
            }
            return false;
        }

        bool condition_check_fcm()
        {
            // check number of flows should be more or equal than degree...
            if (now_flow_num < beta_degree)
                return false; // exit for next comb.


            if (beta_degree == now_flow_num){
                // degree 2, num 2,, or degree 3, num 3
                // layer 1
                for (int i = 0; i < now_flow_num; ++i){
                    if (now_result[i] <= THRESL1)
                        return false;
                }

                // layer 2 
                int num_cond_l2 = thres.size() - beta_degree;
                if (num_cond_l2 == 0){ // if no condition on layer 2
                    return true;
                }
                else if (num_cond_l2 == 1){ // if one condition
                    if (thres[beta_degree][1] != beta_degree){
                        printf("[ERROR] condition is not correct");
                    }

                    int val = 0;
                    for (int j = 0; j < thres[beta_degree][1]; ++j)
                        val += now_result[j];

                    if (val <= thres[beta_degree][2])
                        return false;
                }
                else if (num_cond_l2 == 2) { // if (num_cond_l2 == 2){ // for two conditions
                    int thres_l2_1 = thres[beta_degree][2];
                    int thres_l2_2 = thres[beta_degree + 1][2];
                    if (beta_degree == 2) // degree 2
                        if (now_result[0] <= thres_l2_1 or now_result[1] <= thres_l2_2)
                            return false;
                    else{ // degree 3
                        int val_1 = 0;
                        int val_2 = 0;
                        for (int i = 0; i < thres[beta_degree][1]; ++i)
                            val_1 += now_result[i];
                        for (int i = thres[beta_degree][1]; i < thres[beta_degree][1] + thres[beta_degree+1][1]; ++i)
                            val_2 += now_result[i];

                        if (val_1 <= thres_l2_1 or val_2 <= thres_l2_2)
                            return false;
                    }
                }   
                else { // only when degree 3
                    if (now_result[0] <= thres[beta_degree][2] or now_result[1] <= thres[beta_degree + 1][2] or now_result[2] <= thres[beta_degree + 2][2])
                        return false;
                }
                return true; // if no violation, it is a good permutation!
            }
            else if (beta_degree + 1 == now_flow_num){
                // degree 2, num 3
                // layer 1
                if (now_result[0] + now_result[1] <= THRESL1 or now_result[2] <= THRESL1)
                    return false;

                // layer 2
                int num_cond_l2 = thres.size() - beta_degree;
                if (num_cond_l2 == 0){ // if no condition on layer 2
                    return true;
                }
                else if (num_cond_l2 == 1){ // if one condition
                    int val = 0;
                    for (int j = 0; j < thres[beta_degree][1]; ++j)
                        val += now_result[j];
                    if (val <= thres[beta_degree][2])
                        return false;
                }
                else { // if two conditions
                    if (now_result[0] + now_result[1] <= thres[beta_degree][2] or now_result[2] <= thres[beta_degree + 1][2])
                        return false;
                }
            }
            else if (beta_degree + 2 == now_flow_num){
                // degree 2, num 4
                // layer 1
                if (now_result[3] > THRESL1){
                    if (now_result[0] + now_result[1] + now_result[2] <= THRESL1)
                        return false;
                }
                else {
                    if (now_result[0] + now_result[3] <= THRESL1 or now_result[1] + now_result[2] <= THRESL1)
                        return false;
                }

                // layer 2
                int num_cond_l2 = thres.size() - beta_degree;
                if (num_cond_l2 == 0){ // if no condition
                    return true;
                }
                else if (num_cond_l2 == 1){ // if one condition
                    int val = 0;
                    for (int j = 0; j < thres[beta_degree][1]; ++j)
                        val += now_result[j];
                    if (val <= thres[beta_degree][2])
                        return false;
                }
                else { // if two conditions
                    int thres_l2_1 = thres[beta_degree][2];
                    int thres_l2_2 = thres[beta_degree + 1][2];

                    if (now_result[3] > thres_l2_2){
                        if (now_result[0] + now_result[1] + now_result[2] <= thres_l2_1)
                            return false;
                    }
                    else {
                        if (now_result[0] + now_result[3] <= thres_l2_1 or now_result[1] + now_result[2] <= thres_l2_2)
                            return false;
                    }
                }
            }
            return true; // if no violation, it is a good permutation!
        }

        bool condition_check_fcm_simple4to3()
        {
            // check number of flows should be more or equal than degree...
            if (now_flow_num < beta_degree)
                return false; // exit for next comb.

            // layer 1 check
            for (int i = 0; i < now_flow_num; ++i){
                if (now_result[i] <= THRESL1)
                    return false;
            }

            // layer 2, degree 3
            // In this setting, degree == 3, num_flow == 3
            int num_cond_l2 = thres.size() - simplified;
            if (num_cond_l2 == 0) { // if no layer 2 condition
                return true;
            }
            else if (num_cond_l2 == 1) { // if one condition
                int val = 0;
                for (int j = 0; j < now_flow_num; ++j)
                    val += now_result[j];
                if (val <= THRESL2 + 3 * THRESL1)
                    return false;
            }
            else if (num_cond_l2 == 2) { // if two conditions
                if (now_result[0] + now_result[1] <= THRESL2 + 2 * THRESL1 or now_result[2] <= THRESL2 + THRESL1)
                    return false;
            }
            else { // if num_cond_l2 == 3, 3 conditions
                for (int i = 0; i < now_flow_num; ++i){
                    if (now_result[i] <= THRESL2 + 3 * THRESL1)
                        return false;
                }
            }
            return true;
        }

        bool condition_check_fcm_simple4to2()
        {
            // check number of flows should be more or equal than degree...
            if (now_flow_num < beta_degree)
                return false; // exit for next comb.

            // layer 1 check
            for (int i = 0; i < now_flow_num; ++i){
                if (now_result[i] <= THRESL1)
                    return false;
            }

            // layer 2, degree 2
            // In this setting, degree == 2 and num_flow == 2 (but regard the origianl flow num should be 3, to reduce overestimate)
            int num_cond_l2 = thres.size() - simplified; // here, simplified is origianl degree
            if (num_cond_l2 == 0) { // if no layer 2 condition
                return true;
            }
            else if (num_cond_l2 == 1) { // if one condition
                if (now_result[0] + now_result[1] <= THRESL2 + 3 * THRESL1)
                    return false;
            }
            else { // if two conditions
                if (now_result[0] <= THRESL2 + THRESL1 or now_result[1] <= THRESL2 + 2 * THRESL1)
                    return false;
            }
            return true;
        }

        bool condition_check_fcm_simple2()
        {
            // check number of flows should be more or equal than degree...
            if (now_flow_num < beta_degree)
                return false; // exit for next comb.

            // layer 1 check
            for (int i = 0; i < now_flow_num; ++i){
                if (now_result[i] <= THRESL1)
                    return false;
            }

            // layer 2
            // if (beta_degree == 2){ // if degree 2
            int num_cond_l2 = thres.size() - simplified; // here, simplified is origianl degree
            if (num_cond_l2 == 0) { // if no layer 2 condition
                return true;
            }
            else if (num_cond_l2 == 1) { // if one condition
                if (now_result[0] + now_result[1] <= THRESL2 + 2 * THRESL1)
                    return false;
            }
            else { // if two conditions
                if (now_result[0] <= THRESL2 + THRESL1 or now_result[1] <= THRESL2 + 2 * THRESL1)
                    return false;
            }
            // }
            return true;
        }
    };



    double get_p_from_beta_fcm(BetaGenerator_highdeg & bt, double lambda, vector<double> & now_dist, double now_n, int degree, int d)
    {
        unordered_map<uint32_t, uint32_t> mp;
        for (int i = 0; i < bt.now_flow_num; ++i) {
            mp[bt.now_result[i]]++;
        }
        double ret = std::exp(-lambda);
        for (auto & kv: mp) {
            uint32_t fi = kv.second; // > 0
            uint32_t si = kv.first;
            double lambda_i = now_n * degree * now_dist[si] / W0; // now_dist_i = \[phi_i]
            ret *= (std::pow(lambda_i, fi)) / factorial(fi);
        }
        return ret;
    }



    static constexpr int factorial(int n) {
        if (n == 0 || n == 1)
            return 1;
        return factorial(n - 1) * n;
    };

    void collect_counters(vector<vector<vector<uint32_t> > > counters){
        // aggregate same size of flows with same degree
        counter_dist.resize(DEPTH);
        for (int d = 0; d < DEPTH; ++d){
            counter_dist[d].resize(counters[d].size()); // resize for possible degree size
            for (int xi = 0; xi < counters[d].size(); ++xi){
                // xi is degree, but here we only use xi == 1
                if (counters[d][xi].size() == 0)
                    continue; // stop and go next
                int max_counter_val = (int)(*std::max_element(counters[d][xi].begin(), counters[d][xi].end()));
                this->max_val = (this->max_val > max_counter_val ? this->max_val : max_counter_val);
                counter_dist[d][xi].resize(max_counter_val + 1); // resize for possible value size for each degree
                std::fill(counter_dist[d][xi].begin(), counter_dist[d][xi].end(), 0); // initialize to 0
                for (int j = 0; j < counters[d][xi].size(); ++j)
                    counter_dist[d][xi][counters[d][xi][j]]++; // initialize counter dist (n_i)
                m[d][xi] = counters[d][xi].size(); // number of counters of each degree except empty counters
            }
        }
    }



public:
    EM_FCM(){}
    ~EM_FCM(){}

    void set_counters(vector<vector<vector<uint32_t> > > _newsk, vector<vector<vector<vector<vector<uint32_t> > > > > _newsk_thres)
    {
        this->newsk = _newsk;
        this->newsk_thres = _newsk_thres;
        m.resize(DEPTH);
        for (int d = 0; d< DEPTH; ++d){
            m[d].resize(newsk[d].size(), 0);
            m[d][0] = W0;
        }
        collect_counters(_newsk); // align the input counters

        // Initialize total number of flows
        for (int d = 0; d < DEPTH; ++d)
            for (int i = 1; i < m[d].size(); ++i)
                n_new += m[d][i]; // total number of counters, for initializing distribution
        n_new = n_new / static_cast<double>(DEPTH); // normalize

        /** Initialization **/
        printf("[EM_FCM] Initial Cardinality : %9.1f\n", n_new);
        printf("[EM_FCM] Maximum Counter Value : %8d\n", this->max_val);

        dist_new.resize(max_val + 1);
        ns.resize(max_val + 1);

        // collect and initialize dist_new (ratio) and ns (number)
        for (int d = 0; d < DEPTH; ++d){ // depth
            for (int i = 1; i < counter_dist[d].size(); ++i){ // degree
                for (int j = 1; j < counter_dist[d][i].size(); ++j){ // counter
                    dist_new[j] += counter_dist[d][i][j];
                    ns[j] += counter_dist[d][i][j];
                }
            }
        }
        // normalizing the above ones
        for (int j = 0; j < dist_new.size(); ++j){
            dist_new[j] /= (static_cast<double>(DEPTH) * static_cast<double>(n_new));
            ns[j] /= static_cast<double>(DEPTH);
        }
        printf("[EM_FCM] Counter Setting is finished...\n");
    }

    void epoch_parallel_singledeg(vector<double> &nt_vec, int d, int xi)
    {
        // start EM
        double lambda = n_old * xi / static_cast<double>(W0);
        nt_vec.resize(this->max_val + 1);
        std::fill(nt_vec.begin(), nt_vec.end(), 0.0); // initilize
        for (int i = 1; i < counter_dist[d][xi].size(); ++i){ // start size 1
            if (counter_dist[d][xi][i] == 0)
                continue; // no value, then move next
            // iterate all combinations of size i flows
            BetaGenerator bts1(i), bts2(i);
            
            double sum_p = 0; // denominator
            while (bts1.get_next()){
                sum_p += get_p_from_beta(bts1, lambda, dist_old, n_old, d);
            }

            // for debug
            if (sum_p == 0){
                continue;
            }
            else {
                while (bts2.get_next()){
                    double p = get_p_from_beta(bts2, lambda, dist_old, n_old, d);
                    for (int j = 0; j < bts2.now_flow_num; ++j){
                        nt_vec[bts2.now_result[j]] += counter_dist[d][xi][i] * p / sum_p;
                    }
                }
            }
        }

        double accum = std::accumulate(nt_vec.begin(), nt_vec.end(), 0.0);
        if (counter_dist[d][xi].size() != 0)
            printf("[EM_FCM] ******** depth %2d, degree %2d is finished...(accum:%10.1f, #val:%8d) **********\n", d, xi, accum, (int)newsk[d][xi].size());
    }


    void epoch_parallel_multideg(vector<double> &nt_vec, int d, int xi)
    {
        // start EM
        double lambda = n_old * (xi) / double(m[d][0]);
        nt_vec.resize(this->max_val);
        std::fill(nt_vec.begin(), nt_vec.end(), 0); // initialize
        for (int i = 0; i < newsk[d][xi].size(); ++i){
            if (newsk[d][xi][i] == 0)
                continue;

            /* iterate for all combinations of size newsk[d][xi][i] with threshold newsk_thres[d][xi][i] : vector<vector<uint32_t> > : list of <layer,#paths,threshold> */
            BetaGenerator_highdeg bts1(newsk[d][xi][i], newsk_thres[d][xi][i], xi), bts2(newsk[d][xi][i], newsk_thres[d][xi][i], xi);
            int sum_iter = 0; // number of iteration
            double sum_p = 0; // denominator
            while (bts1.get_next()){
                // double p = get_p_from_beta_fcm(bts1, lambda, dist_old, n_old, xi, d);
                sum_p += get_p_from_beta_fcm(bts1, lambda, dist_old, n_old, xi, d);
                sum_iter += 1;
            }

            if (sum_p == 0){ // for debug
                if (sum_iter > 0){ // adjust the value
                    int temp_val = newsk[d][xi][i]; // value 
                    vector<vector<uint32_t> > temp_thres = newsk_thres[d][xi][i]; // thresholds
                    int temp_n_layer2 = temp_thres.size() - xi; // if layer 2 threshold exists?
                    int temp_threshold_l1 = temp_thres[0][2]; // threshold of layer 1
                    temp_val -= temp_threshold_l1 * (xi - 1); // remove overlaps at layer 1
                    if (temp_n_layer2 > 0){
                        int temp_threshold_l2 = temp_thres[xi][2];
                        temp_val -= temp_threshold_l2 * (temp_n_layer2 - 1);
                    }
                    nt_vec[temp_val] += 1;
                }
            }
            else {
                while (bts2.get_next()){
                    double p = get_p_from_beta_fcm(bts2, lambda, dist_old, n_old, xi, d);
                    for (int j = 0; j < bts2.now_flow_num; ++j){
                        nt_vec[bts2.now_result[j]] += p / sum_p;
                    }
                }
            }
        }
        double accum = std::accumulate(nt_vec.begin(), nt_vec.end(), 0.0);
        if (newsk[d][xi].size() != 0){
            printf("[EM_FCM] ******* depth %2d, degree %2d is finished...(accum:%10.1f, #val:%8d)*******\n", d, xi, accum, (int)newsk[d][xi].size());
        }
    }


    void next_epoch()
    {
        // save dist_new, n_new, ns and re-initialize
        iter++;
        dist_old = dist_new;
        n_old = n_new;
        vector<double> denominator(DEPTH, 0.0);
        std::fill(ns.begin(), ns.end(), 0.0);
        vector<int> num_thread(DEPTH, 0); // for multi-threading

        // get the maximum degree of each degree
        for (int d = 0; d < DEPTH; ++d)
            for (int xi = 0; xi < counter_dist[d].size(); ++xi)
                if (counter_dist[d][xi].size() != 0)
                    num_thread[d] = std::max(num_thread[d], xi);
        int total_num_thread = std::accumulate(num_thread.begin(), num_thread.end(), 0);
        printf("[EM_FSD] Total number of thread : %d\n", total_num_thread);




        // temporary counter values for multi-threading EM
        vector<vector<vector<double> > > nt_d_xi(DEPTH); // d(depth)->xi(degree)->counts(num.flows)
        for (int d = 0; d < DEPTH; ++d)
            nt_d_xi[d].resize(counter_dist[d].size());



        
        /**** MULTI_THREADING ****/
        printf("[EM] Start multi-threading...\n");
        std::thread thrd[total_num_thread];
        int iter_thread = 0;
        for (int d = 0; d < DEPTH; ++d){
            for (int xi = 1; xi <= num_thread[d]; ++xi){ // degree (xi)
                // parallelization
                if (xi == 1){
                    thrd[iter_thread] = std::thread(&EM_FCM::epoch_parallel_singledeg, *this, std::ref(nt_d_xi[d][xi]), d, xi);
                }
                else{
                    thrd[iter_thread] = std::thread(&EM_FCM::epoch_parallel_multideg, *this, std::ref(nt_d_xi[d][xi]), d, xi);
                }
                iter_thread += 1;
            }
        }
        /***   dash until thread is finished ***/
        for (int i = 0; i < total_num_thread; ++i)
            thrd[i].join();
        /**************************/

        // /***** SINGLE_THREADING ****/
        // printf("[EM] Start single-threading...\n");
        // for (int d = 0; d < DEPTH; ++d){
        //     for (int xi = 1; xi <= num_thread[d]; ++xi){
        //         if (xi == 1)
        //             this->epoch_parallel_singledeg(nt_d_xi[d][xi], d, xi);
        //         else
        //             this->epoch_parallel_multideg(nt_d_xi[d][xi], d, xi);
        //     }
        // }
        // /***************************/

        // collect all information
        for (int d = 0; d < DEPTH; ++d)
            for (int xi = 1; xi <= num_thread[d]; ++xi)
                for (int i = 0; i < ns.size(); ++i)
                    ns[i] += nt_d_xi[d][xi][i];

        n_new = 0.0;
        for (int i = 0; i < ns.size(); ++i){
            if (ns[i] != 0)
            {
                ns[i] /= double(DEPTH); // divide the denominator
                n_new += ns[i]; // save the results
            }
        }
        for (int i = 0; i < ns.size(); ++i)
            dist_new[i] = ns[i] / n_new;

        printf("[EM_FCM - iter %2d] Intermediate cardianlity : %9.1f\n\n", iter, n_new);
    }
};


#endif
