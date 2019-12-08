#include "main.h"

#include <iostream>
#include <bearparser/bearparser.h>
#include <beardisasm.h>

#define MINBUF 0x200

using namespace minidis;

FileView* tryLoading(QString &fName)
{
    FileView *fileView = NULL;
    bufsize_t maxMapSize = FILE_MAXSIZE;
    do {
        if (!QFile::exists(fName)) {
            std::cerr << "[ERROR] " << "The file does not exist" << std::endl;
            break;
        }
        try {
            fileView = new FileView(fName, maxMapSize);
        } catch (BufferException &e1) {
            std::cerr << "[ERROR] " << e1.what() << std::endl;
            if (maxMapSize == 0) break;
        }
    } while (!fileView);
    
    return fileView;
}

void disasmPeFile(PEFile *exe, offset_t func_offset)
{
    if (!exe) return;

    const minidis::DisasmSettings basicSettings(0, false, true);
    
    minidis::PeDisasm disasm1(exe);
    disasm1.init(func_offset);
    if (!disasm1.fillTable(basicSettings)) return;
    for (size_t i = 0; ; i++) {
        DisasmChunk* chunk = disasm1.getChunkAtIndex(i);
        if (!chunk) break;
        std::cout << std::hex << disasm1.getOffset(i, Executable::VA)
            << " : "
            << chunk->toString().toStdString();
            
            if (chunk->isBranching()) {
                offset_t target = chunk->getTargetAddr();
                
                std::cout << " -> "
                    << std::hex << exe->convertAddr(target, chunk->getTargetAddrType(), Executable::VA);
                if (disasm1.isImportCall(i)) {
                    std::cout << " ; " << disasm1.getImportName(target, chunk->getTargetAddrType()).toStdString();
                }
                std::cout << "\n----";
            }
            std::cout << "\n";
    }
    
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ExeFactory::init();

    if (argc < 2) {
        std::cout << "Bearparser version: " <<  BEARPARSER_VERSION << "\n";
        std::cout << "Args: <PE file>\n";
        return 0;
    }

    int status = 0;
    QString fName = QString(argv[1]);
    
    
    try {
        FileView* fileView = tryLoading(fName);
        if (!fileView) return -1;

        ExeFactory::exe_type exeType = ExeFactory::findMatching(fileView);
        if (exeType == ExeFactory::NONE) {
           std::cerr << "Type not supported\n";
           ExeFactory::destroy();
           return 1;
        }
        
        std::cout << "Type: " << ExeFactory::getTypeName(exeType).toStdString() << std::endl;
        bufsize_t readableSize = fileView->getContentSize();
        bufsize_t allocSize = (readableSize < MINBUF) ? MINBUF : readableSize;

        //std::cout << "Buffering..." << std::endl;
        ByteBuffer *buf = new ByteBuffer(fileView, 0, allocSize);
        delete fileView;

        //std::cout << "Parsing executable..." << std::endl;
        Executable *exe = ExeFactory::build(buf, exeType);
        
        offset_t ep_raw = exe->getEntryPoint(Executable::RAW);
        PEFile *pe = dynamic_cast<PEFile*>(exe);
        if (pe) {
            disasmPeFile(pe, ep_raw);
        }
        
        delete exe;
        delete buf;

        std::cout << "Bye!" << std::endl;
        
    } catch (CustomException &e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        status = -1;
    }
    ExeFactory::destroy();
    return status;
}
