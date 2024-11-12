//
// Created by Sy on 2024/11/9.
//
#pragma once
#include "optv_c++.h"
#include <memory>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include "TIMER.h"
#include <unordered_set>
#include <random>
class ParallelM
{
public:
    std::shared_ptr<OPTVEnv> env = nullptr;
    std::shared_ptr<OPTVModel> model = nullptr;
    std::unordered_map<std::string,int> process_time_map; //工件_机器 加工时间
    std::mt19937 gen;
    int n = 0; //工件个数
    int m = 0; //机器个数
    std::shared_ptr<OPTVVar> C_max = nullptr;
    std::unordered_map<std::string,std::shared_ptr<OPTVVar>> y_ji; //job j 是否分配到机器i上
    std::unordered_map<std::string,double> root_info;
    int neighborhood_size = 0;
    double alpha = 0.0;

    std::unordered_map<int,int> best_sol;  //工件j分配到机器i上
    double best_obj = 1e20;

    std::unordered_set<std::string> fixed_vars;
    int Timelimit = 1000;
    int Threads = 1;
    double explore_time = 40;

    std::string log_file;
    std::shared_ptr<OPTVConstr> obj_constr = nullptr;
    ParallelM(const std::string& log_file_,double ratio = 0.5,int time_limit_ = 1000,int threads_ = 1)
    {
        env = std::make_shared<OPTVEnv>(log_file_);
        model = std::make_shared<OPTVModel>(*env);
        model->Set(OPTVIntParam::OUTPUT_FLAG, 0);
        alpha = ratio;
        Timelimit = time_limit_;
        Threads = threads_;
        gen.seed(1024);
        log_file = log_file_;
        //打开log_file并清空
        std::ofstream outfile(log_file,std::ios::out);
        outfile.close();
    }

    void loadinstancebyfile(const std::string& file_path)
    {
        std::ifstream infile(file_path);
        if (!infile)
        {
            std::cerr << "Error opening file: " << file_path << std::endl;
            return;
        }

        // Read the number of jobs (n)
        infile >> n;
        // Read the number of machines (m)
        infile >> m;
        // Ignore the rest of the line after reading m
        infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Loop over each job
        for (int i = 0; i < n; ++i)
        {
            std::string line;
            // Read the line containing processing times for job i
            if (!std::getline(infile, line))
            {
                std::cerr << "Error reading line for job " << i << std::endl;
                return;
            }
            std::istringstream iss(line);
            // Loop over each machine
            for (int j = 0; j < m; ++j)
            {
                int process_time;
                // Read the processing time for machine j
                if (!(iss >> process_time))
                {
                    std::cerr << "Error reading process time for job " << i << ", machine " << j << std::endl;
                    return;
                }
                // Create the key as "job_machine" and store the processing time
                std::string key = std::to_string(i) + "_" + std::to_string(j);
                process_time_map[key] = process_time;
            }
        }
    }

    void report()
    {
        std::cout << "Number of jobs: " << n << std::endl;
        std::cout << "Number of machines: " << m << std::endl;
        std::cout << "Processing times size :"<<process_time_map.size() << std::endl;
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < m; ++j)
            {
                std::string key = std::to_string(i) + "_" + std::to_string(j);
                std::cout << "Job " << i << ", Machine " << j << ": " << process_time_map[key] << std::endl;
            }
        }
    }

    std::pair<int,int> extractnum(const std::string& str)
    {
        std::istringstream iss(str);
        std::string segment;
        int firstNumber, secondNumber;

        // 获取第一个数字
        if (std::getline(iss, segment, '_')) {
            firstNumber = std::stoi(segment);
        }

        // 获取第二个数字
        if (std::getline(iss, segment, '_')) {
            secondNumber = std::stoi(segment);
        }
        return {firstNumber,secondNumber};
    }

    void createmodel()
    {
        neighborhood_size = int(alpha * n);
        int bigM = 0;
        for (int j = 0; j < n; ++j)
        {
            for (int i = 0; i < m; ++i)
            {
                std::string key = std::to_string(j) + "_" + std::to_string(i);
                y_ji[key] = std::make_shared<OPTVVar>(model->AddVar(0, 1,0,OPTV_BINARY,key));
                bigM += process_time_map[key];
            }
        }
        C_max = std::make_shared<OPTVVar>(model->AddVar(0, bigM,1,OPTV_CONTINUOUS,"C_max"));
        // Add the constraints
        for (int j = 0; j < n; ++j)
        {
            OPTVLinExpr sum_expr;
            for (int i = 0; i < m; ++i)
            {
                std::string key = std::to_string(j) + "_" + std::to_string(i);
                sum_expr += (*y_ji[key]);  // 将每个变量添加到表达式中
            }

            // 添加约束，使得每个工件只能分配到一个机器上
            model->AddConstr(sum_expr,1,1, "JobAssignment_" + std::to_string(j));
        }
        for (int i = 0; i < m; ++i)
        {
            OPTVLinExpr sum_expr;
            for (int j = 0; j < n; ++j)
            {
                std::string key = std::to_string(j) + "_" + std::to_string(i);
                sum_expr += process_time_map[key] * (*y_ji[key]);
            }
            model->AddConstr(sum_expr - *C_max,-OPTV_INF ,0,"Machine_" + std::to_string(i));
        }
    }

    void addTimelimt(double time_limit)
    {
        time_limit = std::max(0.01,time_limit);
        model->Set(OPTVDblParam::TIME_LIMIT, time_limit);
    }

    void addThreads(int nthreads)
    {
        model->Set(OPTVIntParam::THREADS, nthreads);
    }

    void fixvars()
    {
        for (const auto& var_name : fixed_vars)
        {
            fixvar(var_name,1);
        }
        model->Update();
    }

    void freevars()
    {
        for (const auto& var_name : fixed_vars)
        {
            freevar(var_name);
        }
        model->Update();
    }

    void fixvar(const std::string& var_name,int value)
    {
        y_ji[var_name]->Set(OPTVDblAttr::LB, value);
        y_ji[var_name]->Set(OPTVDblAttr::UB, value);

    }

    void freevar(const std::string& var_name)
    {
        y_ji[var_name]->Set(OPTVDblAttr::LB, 0);
        y_ji[var_name]->Set(OPTVDblAttr::UB, 1);
    }

    bool getrootinfo(int time_limit,int threads)
    {
        bool flag = false;
        auto relaxedModel = model->Relax();
        relaxedModel.Set(OPTVDblParam::TIME_LIMIT, time_limit);
        relaxedModel.Set(OPTVIntParam::THREADS, threads);
        relaxedModel.Optimize();
        if (relaxedModel.Get(OPTVIntAttr::SOL_COUNT) > 0)
        {
            flag = true;
            std::cout << "Optimal solution for relaxed model found." << std::endl;
            std::cout << "Objective Value: " << relaxedModel.Get(OPTVDblAttr::OBJ_VAL) << std::endl;
            // Get the values of the variables
            for (int j = 0; j < n; ++j)
            {
                for (int i = 0; i < m; ++i)
                {
                    std::string key = std::to_string(j) + "_" + std::to_string(i);
                    root_info[key] = relaxedModel.GetVarByName(key).Get(OPTVDblAttr::X);
//                    std::cout << key << " root value : " << root_info[key] << std::endl;
                }
            }
        }
        return flag;
    }

    void solve()
    {
        model->Optimize();
        if (model->Get(OPTVIntAttr::SOL_COUNT) > 0)
        {
//            std::cout << "Optimal solution found." << std::endl;
//            std::cout << "Objective Value: " << model->Get(OPTVDblAttr::OBJ_VAL) << std::endl;
            // Get the values of the variables
            for (int j = 0; j < n; ++j)
            {
                for (int i = 0; i < m; ++i)
                {
                    std::string key = std::to_string(j) + "_" + std::to_string(i);
                    double value = model->GetVarByName(key).Get(OPTVDblAttr::X);
                    if (value > 0.5)
                    {
                        best_sol[j] = i;
                    }
                }
            }
            best_obj = model->Get(OPTVDblAttr::OBJ_VAL);
            if (best_obj < 1e20)
            {
                std::ofstream outfile(log_file,std::ios::app);
                outfile <<TIMER::ClickMs()<<","<< best_obj << std::endl;
                outfile.close();
                if (obj_constr != nullptr)
                {
                    model->Remove(*obj_constr);
                }
                obj_constr = std::make_shared<OPTVConstr>(model->AddConstr(*C_max, -OPTV_INF,best_obj-1,"ObjConstr"));
                model->Update();
            }
        }
    }

    void getinitsol()
    {
        std::vector<std::pair<std::string,double>> scores;
        for (int j = 0; j < n; ++j)
        {
            double max_score = 0;
            int max_index = 0;
            for (int i = 0; i < m; ++i)
            {
                std::string key = std::to_string(j) + "_" + std::to_string(i);
                double score = root_info[key];
                if (score > max_score)
                {
                    max_score = score;
                    max_index = i;
                }
            }
            scores.push_back({std::to_string(j) + "_" + std::to_string(max_index),max_score});
        }
        //scores根据第二位从大到小排序
        std::sort(scores.begin(),scores.end(),[](const std::pair<std::string,double>& a,const std::pair<std::string,double>& b){
            return a.second > b.second;
        });
        fixed_vars.clear();
        for (int i = 0; i < n-0.5*neighborhood_size; ++i)
        {
            fixed_vars.insert(scores[i].first);
        }
    }

    void getfixed()
    {
        std::vector<std::pair<int,int>> machine_acc_time;
        std::unordered_map<int,std::vector<int>> machine_jobs;
        for (int i = 0; i < m; ++i)
        {
            machine_acc_time.push_back({i,0});
        }
        for (const auto& tmp_sol:best_sol)
        {
            int job = tmp_sol.first;
            int machine = tmp_sol.second;
            machine_jobs[machine].push_back(job);
            std::string key = std::to_string(job) + "_" + std::to_string(machine);
            machine_acc_time[machine].second += process_time_map[key];
        }
        //根据第二位从大到小排序
        std::sort(machine_acc_time.begin(),machine_acc_time.end(),[](const std::pair<int,int>& a,const std::pair<int,int>& b){
            return a.second > b.second;
        });
        //destroy neighborhood_size 个工件
        std::unordered_set<int> destroy_jobs;
        for (int i = 0; i < machine_acc_time.size(); ++i)
        {
            destroy_jobs.insert(machine_jobs[machine_acc_time[i].first].begin(),machine_jobs[machine_acc_time[i].first].end());
            if (destroy_jobs.size() > 1.2*neighborhood_size)
            {
                break;
            }
        }
        //从destroy_jobs中随机选择neighborhood_size个工件
        std::vector<int> destroy_jobs_vec(destroy_jobs.begin(),destroy_jobs.end());
        std::shuffle(destroy_jobs_vec.begin(),destroy_jobs_vec.end(),gen);
        destroy_jobs.clear();
        for (int i = 0; i < neighborhood_size; ++i)
        {
            destroy_jobs.insert(destroy_jobs_vec[i]);
        }
        fixed_vars.clear();
        for (int job = 0; job < n; ++job)
        {
            if (destroy_jobs.find(job) == destroy_jobs.end())
            {
                int belong_machine = best_sol[job];
                std::string key = std::to_string(job) + "_" + std::to_string(belong_machine);
                fixed_vars.insert(key);
            }
        }
    }

    void getfixedall()
    {
        // 计算每台机器的负载和对应的工件安排
        std::vector<double> machine_loads(m, 0.0);
        std::unordered_map<int, std::vector<int>> machine_jobs; // 机器i上的工件列表

        for (const auto& sol : best_sol)
        {
            int job = sol.first;
            int machine = sol.second;
            std::string key = std::to_string(job) + "_" + std::to_string(machine);
            machine_jobs[machine].push_back(job);
            machine_loads[machine] += process_time_map[key];
        }

        // 计算总负载
        double total_load = 0.0;
        for (double load : machine_loads)
        {
            total_load += load;
        }

        // 为每个工件分配权重，机器负载越大，工件被选中的概率越高
        std::vector<int> jobs_list;       // 所有工件的列表
        std::vector<double> weights;      // 对应的权重

        for (int i = 0; i < m; ++i)
        {
            double machine_weight = machine_loads[i]; // 机器的负载作为权重
            for (int job : machine_jobs[i])
            {
                jobs_list.push_back(job);
                weights.push_back(machine_weight);
            }
        }

        // 执行无放回的加权随机抽样，选出 neighborhood_size 个工件
        std::unordered_set<int> destroy_jobs;
        std::vector<double> cumulative_weights(weights.size(), 0.0);
        std::partial_sum(weights.begin(), weights.end(), cumulative_weights.begin());

        while (destroy_jobs.size() < neighborhood_size && !jobs_list.empty())
        {
            // 生成一个0到总权重之间的随机数
            std::uniform_real_distribution<double> dist(0.0, cumulative_weights.back());
            double rand_weight = dist(gen);

            // 二分查找找到对应的工件
            auto it = std::lower_bound(cumulative_weights.begin(), cumulative_weights.end(), rand_weight);
            int idx = std::distance(cumulative_weights.begin(), it);
            int selected_job = jobs_list[idx];

            destroy_jobs.insert(selected_job);

            // 移除已选中的工件和对应的权重
            jobs_list.erase(jobs_list.begin() + idx);
            weights.erase(weights.begin() + idx);
            cumulative_weights.erase(cumulative_weights.begin() + idx);

            // 更新累计权重
            for (size_t k = idx; k < cumulative_weights.size(); ++k)
            {
                cumulative_weights[k] -= weights[idx];
            }
        }

        // 固定未被选中的工件对应的变量
        fixed_vars.clear();
        for (int job = 0; job < n; ++job)
        {
            if (destroy_jobs.find(job) == destroy_jobs.end())
            {
                int assigned_machine = best_sol[job];
                std::string key = std::to_string(job) + "_" + std::to_string(assigned_machine);
                fixed_vars.insert(key);
            }
        }

    }

    void getfixedall2()
    {
        // 计算每台机器的负载和对应的工件安排
        std::vector<double> machine_loads(m, 0.0);
        std::unordered_map<int, std::vector<int>> machine_jobs; // 机器i上的工件列表

        for (const auto& sol : best_sol)
        {
            int job = sol.first;
            int machine = sol.second;
            std::string key = std::to_string(job) + "_" + std::to_string(machine);
            machine_jobs[machine].push_back(job);
            machine_loads[machine] += process_time_map[key];
        }

        // 初始化要破坏的工件集合
        std::unordered_set<int> destroy_jobs;

        // 开始循环，直到选出的工件数量等于 neighborhood_size
        while (destroy_jobs.size() < neighborhood_size)
        {
            // 找到负载最大的机器
            int max_load_machine = -1;
            double max_load = -1.0;
            for (int i = 0; i < m; ++i)
            {
                if (machine_loads[i] > max_load && !machine_jobs[i].empty())
                {
                    max_load = machine_loads[i];
                    max_load_machine = i;
                }
            }

            // 如果所有机器的工件都已被移除，无法继续，跳出循环
            if (max_load_machine == -1)
            {
                break;
            }

            // 从负载最大的机器中随机选择一个工件
            std::vector<int>& jobs_on_machine = machine_jobs[max_load_machine];
            std::uniform_int_distribution<int> dist(0, jobs_on_machine.size() - 1);
            int idx = dist(gen);
            int selected_job = jobs_on_machine[idx];

            // 将选中的工件添加到破坏集合中
            destroy_jobs.insert(selected_job);

            // 更新机器的负载
            std::string key = std::to_string(selected_job) + "_" + std::to_string(max_load_machine);
            machine_loads[max_load_machine] -= process_time_map[key];

            // 从机器的工件列表中移除该工件
            jobs_on_machine.erase(jobs_on_machine.begin() + idx);
        }

        // 固定未被选中的工件对应的变量
        fixed_vars.clear();
        for (int job = 0; job < n; ++job)
        {
            if (destroy_jobs.find(job) == destroy_jobs.end())
            {
                int assigned_machine = best_sol[job];
                std::string key = std::to_string(job) + "_" + std::to_string(assigned_machine);
                fixed_vars.insert(key);
            }
        }
    }


    void end2endsolve(int time_limit,int threads)
    {
        createmodel();
        addTimelimt(time_limit);
        addThreads(threads);
        solve();
    }

    void checkfixed()
    {
        int total_vars_num = 0;
        int fixed_vars_num = 0;
        auto vars = model->GetVars();
        for (const auto& var : vars)
        {
            double lb = var.Get(OPTVDblAttr::LB);
            double ub = var.Get(OPTVDblAttr::UB);
            total_vars_num++;
            if (std::abs(lb - ub) < 1e-3)
            {
                fixed_vars_num++;
            }
        }
        std::cout << "Fixed var num : "<<fixed_vars_num<<" / "<<total_vars_num<<std::endl;
    }

    void warmstart()
    {
        for (int j = 0; j < n; ++j)
        {
            for (int i = 0; i < m; ++i)
            {
                auto& var = y_ji[std::to_string(j) + "_" + std::to_string(i)];
                if (std::abs(best_sol[j]-i) < 1e-3)
                {
//                    std::cout << "Fix var : "<<j<<" "<<i<<std::endl;
                    var->Set(OPTVDblAttr::START, 1);
                }
                else
                {
//                    std::cout << "Free var : "<<j<<" "<<i<<std::endl;
                    var->Set(OPTVDblAttr::START, 0);
                }
            }
        }
        model->Update();
    }


    void run()
    {
        createmodel();
        auto root_flag = getrootinfo(Timelimit,Threads);
        if (!root_flag)
        {
            std::cout << "No optimal solution found for the relaxed model." << std::endl;
            return;
        }
        getinitsol();
        fixvars();
        checkfixed();
        addThreads(Threads);
        addTimelimt(TIMER::GetRemainingTime());
        solve();
        std::cout << "Initial solution found. "<<best_obj << std::endl;
        freevars();
        int iter = 0;
        while (!TIMER::isStop())
        {
            iter++;
//            getfixed();
            getfixedall2();
            fixvars();
            checkfixed();
            addThreads(Threads);
            double tmp_time = std::min(explore_time,TIMER::GetRemainingTime());
            addTimelimt(tmp_time);
            warmstart();
            solve();
            std::cout << "Iter : "<<iter<<" , Obj : "<<best_obj << std::endl;
            freevars();
        }
    }
};
