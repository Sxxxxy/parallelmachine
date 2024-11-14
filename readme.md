# 算法设计
并行机(parallel machine)调度问题为将$n$个作业分配到$m$台机器上, 作业$j$在机器$i$上的加工时间为$p_{ji}$, 决策变量$y_{ji} \in \{0,1\}$表示作业$j$是否在机器$i$上加工, 目标是最小化最大完工时间$C_{max}$. 具体的MIP模型如下:
$$
\min  C_{\max}
\\
s.t. \sum_{i\in \mathcal{M}}{y_{ji}}=1 \forall j\in \mathcal{J}
\\
\,\,     C_{\max}\ge \sum_{j\in \mathcal{J}}{p_{ji}y_{ji}}\,\,\forall i\in \mathcal{M}
\\
\,\,    y_{ji}\in \left\{ 0,1 \right\} \,\,\forall j\in \mathcal{J} ,\forall i\in \mathcal{M}
$$
本文以OPTV为内核求解器，分别设计了构造启发式算法greedy lp round(glr)和基于邻域搜索的局部搜索算法local search(ls). 并通过实验比较了两种算法的性能.
## greedy lp round(glr)
glr以MIP的线性松弛解$\tilde{y}$为起点, 将$\alpha$个作业分配到机器上. 具体步骤如下: 首先求解MIP的线性松弛问题, 得到线性松弛解$\tilde{y}$. 然后遍历每个作业, 对于作业$j$, 找到$\tilde{y}_{jk_j}=\max_{k\in \mathcal{M}} \tilde{y}_{jk}$,
然后将最大的前$\alpha$个$\tilde{y}_{jk_j}$做如下操作: 将原始MIP模型中的决策变量$y_{jk_j}$固定为1, 然后使用OPTV求解器求解缩减后的MIP模型, 得到初始的整数可行解$\bar{y}$.
## local search
ls以glr获得的初始解$\bar{y}$为起点, 每次迭代通过添加约束或者固定决策变量, 缩减问题规模, 使模型更容易求解. 本文设计了两种邻域, 分别为local branching邻域和random fix邻域. local branching邻域通过添加local branching约束限制模型中01决策变量翻转的总次数, local branching约束如下:
$$
\sum_{j\in \mathcal{J} ,i\in \mathcal{M} :\bar{y}_{ji}=0}{y_{ji}}+\sum_{j\in \mathcal{J} ,i\in \mathcal{M} :\bar{y}_{ji}=1}{\left( 1-y_{ji} \right)}\le \beta
$$
random fix邻域的构造过程如下: 首先确定被destroy的工件个数$\gamma$, 然后不断迭代, 每次迭代时找到最大负载的机器, 然后从中随机选择一个工件加入destroy集合, 更新该机器负载, 直到destroy集合中的工件个数等于$\gamma$. 下一步将MIP模型中不在destroy集合中的工件固定到当前最好解对应的机器上, 从而构造出缩减的MIP问题, 然后求解.
为了避免算法陷入循环, 在local search阶段交替使用local branching和random fix邻域. 主要使用local branching邻域, 当进行邻域搜索后如果找不到更好的解, 则切换到random fix邻域, 直到找到更好的解, 此时再切换到local branching邻域.
# 数值实验
实验环境为Ubuntu 22.04.4 LTS操作系统, Intel(R) Xeon(R) Gold 6248R CPU @ 3.00GHz CPU, 1.0T RAM. 内核求解器为OptVerse Optimizer version 1.5.2. 数据集来源为Parallel Machine Scheduling标准数据集, 论文为"Mixed-Integer Programming Versus Constraint Programming for Shop Scheduling Problems: New Results and Outlook".