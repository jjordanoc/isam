#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#define PAGE_SIZE 512

template<typename KeyType>
const long M = (PAGE_SIZE - sizeof(int) - sizeof(long)) / (sizeof(KeyType) + sizeof(long));

template<typename KeyType>
struct IndexPage {
    // PAGE_SIZE = sizeof(KeyType) * M + sizeof(long) * (M+1) + sizeof(int)

    KeyType *keys = nullptr;
    long *children = nullptr;
    int size = 0;

    IndexPage() {
        keys = new KeyType[M<KeyType>];
        children = new long[M<KeyType> + 1];
    }

    void read(fstream &file) {
        int i = 0, j = 0;
        KeyType tmpKey;
        long tmpChild;
        file.read((char *) &tmpChild, sizeof(long));
        children[j++] = tmpChild;
        while (size < M<KeyType> && !file.eof()) {
            file.read((char *) &tmpKey, sizeof(KeyType));
            file.read((char *) &tmpChild, sizeof(long));
            if (!file.eof()) {
                keys[i++] = tmpKey;
                children[j++] = tmpChild;
                ++size;
            }

        }
    }

    long locate(KeyType key) {
        int i = 0;
        while (i < size && key <= keys[i]) {
           ++i;
        }
        return children[i];
    }

    vector<long> localizarEntre(KeyType begin_key, KeyType end_key){
        int beginPos;
        int endPos;
        vector<long> result;                // ---From here
        for(int i = 0; i < sizeof(keys); ++i){ // P0 K1 P1

            if(begin_key <= keys[i]){
                //begin_key == keys[i] ? beginPos = i : beginPos = i-1;
                beginPos = i-1;
                break;
            }
        }
        beginPos++;
        for(int i = beginPos; i < sizeof(keys); ++i){
            if(end_key <= keys[i]){
                //end_key == keys[i] ? endPos = i : endPos = i-1;
                endPos = i-1;
                break;
            }
        }
        endPos++;

        //get positions
        for(int i = beginPos; i < endPos+1; ++i){
            result.push_back(children[i]);
        }

        return result;
    }
};

template<typename RecordType>
const long N = (PAGE_SIZE - sizeof(int) - sizeof(long)) / sizeof(RecordType);

template<typename RecordType, typename KeyType>
struct DataPage {
    // PAGE_SIZE = sizeof(RecordType) * N + sizeof(int) + sizeof(long)
    RecordType *records = nullptr;
    int size = 0;
    long next = -1;

    DataPage() {
        records = new RecordType[N<RecordType>];
    }

    void read(fstream &file) {
        int i = 0;
        RecordType tmpRecord;
        file.read((char *) &tmpRecord, sizeof(RecordType));
        while (size < N<RecordType> && !file.eof()) {
            if (!file.eof()) {
                records[i++] = tmpRecord;
                ++size;
            }
            file.read((char *) &tmpRecord, sizeof(RecordType));
        }
        file.read((char *) &next, sizeof(long));
    }

    void write(fstream &file) {
        for (int i = 0; i < size; ++i) {
            file.write((char *) &records[i], sizeof(RecordType));
        }
        file.write((char *) &next, sizeof(long));
    }

    vector<RecordType> locate(KeyType key) {

    }

    vector<RecordType> locateTill(KeyType begin_key, KeyType end_key){
        vector<RecordType> result;


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
        indexPage1.read(index1);
        long page1 = indexPage1.locate(index1, key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        indexPage2.read(index2);
        long page2 = indexPage2.locate(index2, key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        indexPage3.read(index3);
        long page3 = indexPage3.locate(index3, key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        dataPage.read(file);
        if (dataPage.size == N<RecordType>) {
            dataPage.next = file.tellg();
            file.seekp(dataPage.next);
            dataPage = DataPage<RecordType, KeyType>();
            dataPage.records[0] = record;
            dataPage.size = 1;
            dataPage.write(file);
        } else {
            dataPage.records[dataPage.size++] = record;
            file.seekp(page3);
            dataPage.write(file);
        }
        file.close();
    }
    vector<RecordType> search(KeyType key) {
        // read index file 1
        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        indexPage1.read(index1);
        long page1 = indexPage1.locate(index1, key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        indexPage2.read(index2);
        long page2 = indexPage2.locate(index2, key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        indexPage3.read(index3);
        long page3 = indexPage3.locate(index3, key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        dataPage.read(file);
        vector<RecordType> records = dataPage.locate(key);
        file.close();
        return records;
    }


    vector<RecordType> rangeSearch(KeyType begin_key, KeyType end_key) {

        index1.open(indexfilename(1), flags);
        IndexPage<KeyType> indexPage1;
        indexPage1.read(index1);
        long page1 = indexPage1.locate(index1, begin_key);
        index1.close();

        // read index file 2
        index2.open(indexfilename(2), flags);
        index2.seekg(page1);
        IndexPage<KeyType> indexPage2;
        indexPage2.read(index2);
        long page2 = indexPage2.locate(index2, begin_key);
        index2.close();

        // read index file 3
        index3.open(indexfilename(3), flags);
        index3.seekg(page2);
        IndexPage<KeyType> indexPage3;
        indexPage3.read(index3);
        long page3 = indexPage3.locate(index3, begin_key);
        index3.close();

        file.open(filename, flags);
        file.seekg(page3);
        DataPage<RecordType, KeyType> dataPage;
        dataPage.read(file);
        vector<RecordType> records = dataPage.locateTill(end_key);
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

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
