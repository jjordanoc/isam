#include <iostream>

#define PAGE_SIZE 512


template<typename T>
struct IndexPage {
    // PAGE_SIZE = sizeof(T) * M + sizeof(long) * (M+1) + sizeof(int)
    const long M = (PAGE_SIZE - sizeof(int) - sizeof(long)) / (sizeof(T) + sizeof(long));
    T *key = nullptr;
    long *children = nullptr;
    int size = 0;

    IndexPage() {
        key = new T[M];
        children = new long[M + 1];
    }
};

template<typename Record>
struct DataPage {
    // PAGE_SIZE = sizeof(Record) * N + sizeof(int) + sizeof(long)
    const long N = (PAGE_SIZE - sizeof(int) - sizeof(long)) / sizeof(Record);
    Record *records = nullptr;
    int size = 0;
    long next = -1;

    DataPage() {
        records = new Record[N];
    }
};

class ISAM {

};

/*
 * search:
 * open(index1.dat)
 * IndexPage index1;
 * read(index1, sizeof(IndexPage))
 * haga busqueda lineal hasta encontrar por donde bajar
 * pagina1 = localizar(index1, searchKey)
 * open(index2.dat)
 * IndexPage index2;
 * seek(pagina1)
 * read(index2, sizeof(IndexPage))
 * pagina2 = localizar(index2, searchKey)
 * open(index3.dat)
* IndexPage index3;
* seek(pagina1)
* read(index3, sizeof(IndexPage))
* pagina3 = localizar(index3, searchKey)
 * open(data.dat)
 * DataPage dataPage;
 * seek(pagina3)
 * read(dataPage, sizeof(DataPage))
 * registro = localizar(dataPage, searchKey)
 */

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
