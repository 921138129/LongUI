#include "script.h"

// �����а���
static const char* CommandHelp = R"longui([HELP]:
     -h -help       : show help content
    [-o] <filename> : output the C++ source code to this file
)longui";


// Ӧ�ó������
int main(int argc, const char* argv[]) {
    InterfaceScriptReader reader;
    reader.read(LongUIInterface);
    // ������
    if (argc == 1 || *reinterpret_cast<const uint16_t*>(argv[1]) == 'h-' ||
        *reinterpret_cast<const uint32_t*>(argv[1]) == 'leh-') {
        printf(CommandHelp);
    }
    std::getchar();
    return EXIT_SUCCESS;
}