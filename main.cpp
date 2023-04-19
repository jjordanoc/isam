#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#define PAGE_SIZE 512

// PAGE_SIZE = sizeof(KeyType) * M + sizeof(long) * (M+1) + sizeof(int)
template<typename KeyType>
const long M = (PAGE_SIZE - sizeof(int) - sizeof(long)) / (sizeof(KeyType) + sizeof(long));

#define M M<KeyType>

template<typename KeyType>
struct IndexPage {
    int size = 0;
    KeyType keys[M];
    long children[M + 1];

    IndexPage() = default;

    long locate(KeyType key) {
        int i = 0;
        while (i < size && key > keys[i]) {
            ++i;
        }
        return children[i];
    }
};

// PAGE_SIZE = sizeof(RecordType) * N + sizeof(int) + sizeof(long)
template<typename RecordType>
const long N = (PAGE_SIZE - sizeof(int) - sizeof(long)) / sizeof(RecordType);

#define N N<RecordType>

template<typename RecordType, typename KeyType>
struct DataPage {
    int size = 0;
    RecordType records[N];
    long next = -1;

    DataPage() = default;

    vector<RecordType> locate(fstream &file, KeyType key) {
        vector<RecordType> result;
        while (true) {
            for (int i = 0; i < size; ++i) {
                if (records[i].key == key) {
                    result.push_back(records[i]);
                }
            }
            if (next != -1) {
                file.seekg(next);
                file.read((char *) this, PAGE_SIZE);
            } else {
                break;
            }
        }
        return result;
    }

    vector<RecordType> locateRange(fstream &file, KeyType begin_key, KeyType end_key) {
        vector<RecordType> result;

        int i = 0;
        while (i < size && begin_key >= records[i]) {
            ++i;
        }

        bool flag = true;
        for (int j = i; j < sizeof(records); ++j) {
            if (end_key < records[j]) {
                flag = false;
                break;
            }
            result.push_back(records[j]);
        }

        while (flag) {
            file.seekg(next);
            DataPage<RecordType, KeyType> dataPage;
            dataPage.read(file);
            for (int j = 0; j < sizeof(dataPage.records); ++j) {
                if (end_key < records[j]) break;
                result.push_back(records[j]);
            }
        }

        return result;
    }
};

template<typename RecordType, typename KeyType>
class ISAM {
    fstream file;
    string filename;
    fstream index1;
    fstream index2;
    fstream index3;
    string indexfilename(int number) {
        return "index" + to_string(number) + ".dat";
    }
    _Ios_Openmode flags = ios::out | ios::app | ios::binary;

public:
    ISAM(const string &filename) : filename(filename) {
        file.open(filename, flags);
        file.close();
    }
    void add(KeyType key, RecordType record) {
        // read index file 1
        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        index1.read((char *) &indexPage1, PAGE_SIZE);
        long page1 = indexPage1.locate(index1, key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        index2.read((char *) &indexPage2, PAGE_SIZE);
        long page2 = indexPage2.locate(index2, key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        index3.read((char *) &indexPage3, PAGE_SIZE);
        long page3 = indexPage3.locate(index3, key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        file.read((char *) &dataPage, PAGE_SIZE);
        // there are two options, we either
        // 1. find a non-full page in the linked list of pages, in which case we insert it to the end of the non-full page and re-write it
        // 2. all pages are full, in which case we have to create a new page in the linked list of pages
        if (dataPage.size == N - 1) {
            dataPage.next = file.tellg();
            file.seekp(dataPage.next);
            dataPage = DataPage<RecordType, KeyType>();
            dataPage.records[0] = record;
            dataPage.size = 1;
            file.write((char *) &dataPage, PAGE_SIZE);
        } else {
            dataPage.records[dataPage.size++] = record;
            file.seekp(page3);
            file.write((char *) &dataPage, PAGE_SIZE);
        }
        file.close();
    }
    vector<RecordType> search(KeyType key) {
        // read index file 1
        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        index1.read((char *) &indexPage1, PAGE_SIZE);
        long page1 = indexPage1.locate(key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        index2.read((char *) &indexPage2, PAGE_SIZE);
        long page2 = indexPage2.locate(key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        index3.read((char *) &indexPage3, PAGE_SIZE);
        long page3 = indexPage3.locate(key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        file.read((char *) &dataPage, PAGE_SIZE);
        vector<RecordType> records = dataPage.locate(file, key);
        file.close();
        return records;
    }


    vector<RecordType> rangeSearch(KeyType begin_key, KeyType end_key) {
        // read index file 1
        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        index1.read((char *) &indexPage1, PAGE_SIZE);
        long page1 = indexPage1.locate(begin_key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        index2.read((char *) &indexPage2, PAGE_SIZE);
        long page2 = indexPage2.locate(begin_key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        index3.read((char *) &indexPage3, PAGE_SIZE);
        long page3 = indexPage3.locate(begin_key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        file.read((char *) &dataPage, PAGE_SIZE);
        vector<RecordType> records = dataPage.locateRange(file, begin_key, end_key);
        file.close();
        return records;
    }
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

struct Alumno {
    char codigo[10];
    char nombre[40];
    int ciclo;
};

int main() {
    ISAM<Alumno, char[10]> isam("data.dat");
    return 0;
}
