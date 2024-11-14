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
ls以glr获得的初始解$\bar{y}$为起点, 每次迭代不断固定$\beta$个决策变量, 求解缩减后的MIP模型.
# 数值实验
