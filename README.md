# InvertedIndex_OpenMP
OpenMP并行化编程实验

ListWiseOMP0:
    仅仅在GetIntersectionAndAnswer部分进行了OMP并行化处理，
    每一次计算GetIntersectionAndAnswer都需要重新创建线程

ListWiseOMP1:
    将ReadIndex的位向量转换部分也进行了OMP并行化

ListWiseOMP2:
    在OMP1的基础上，
    尝试为所有的查询只创建一次线程
    通过barrier同步

ListWiseOMP2_SIMD:
    ListWiseOMP2 + SIMD

ListWiseOMP3:
    负载均衡调度

ListWiseOMP3_SIMD:
    ListWiseOMP3 + SIMD
