// tmp_script/test_ep_ip_ipc.cpp
#include "../ipc/protocol/IpcFrame.h"
#include "../ipc/protocol/IpcFrameCodec.h"
#include "../ipc/protocol/IpcConstants.h"
#include "../remotediag/sid_filter/SIDFilter.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "[TEST] 1. Testing IpcFrameCodec Encode/Decode..." << std::endl;
    
    IpcFrame frame;
    frame.frameType = IpcFrame::TYPE_REQUEST;
    frame.messageId = CMD_DIAG_GET_RDG_FLAG;
    frame.correlationId = 12345;
    frame.payload = {0x01, 0x02, 0x03, 0x04};
    frame.payloadLen = 4;

    auto encoded = IpcFrameCodec::encode(frame);
    std::cout << "  Encoded size: " << encoded.size() << " bytes" << std::endl;

    IpcFrame decodedFrame;
    size_t consumed = IpcFrameCodec::decode(encoded.data(), encoded.size(), decodedFrame);
    assert(consumed == encoded.size());
    assert(decodedFrame.frameType == IpcFrame::TYPE_REQUEST);
    assert(decodedFrame.messageId == CMD_DIAG_GET_RDG_FLAG);
    assert(decodedFrame.correlationId == 12345);
    assert(decodedFrame.payloadLen == 4);
    assert(decodedFrame.payload.size() == 4);
    assert(decodedFrame.payload[0] == 0x01);

    std::cout << "  Codec decode test PASSED!" << std::endl;

    std::cout << "[TEST] 2. Testing SIDFilter..." << std::endl;
    auto filter = SIDFilter::getInstance();
    
    // Allowed SID 0x22 (ReadDataByIdentifier)
    bool allowed22 = filter->filterAndSendUdsData(1, {0x22, 0xF1, 0x90});
    assert(allowed22 == true);

    // Blocked SID 0x35 (RequestDownload - not in whitelist)
    bool allowed35 = filter->filterAndSendUdsData(1, {0x35, 0x00});
    assert(allowed35 == false);

    // Test isAllowedUdsData pointer overload (used by RemoteDirectCommand)
    uint8_t rawUds22[] = {0x22, 0xF1, 0x90};
    uint8_t rawUds35[] = {0x35, 0x00};
    assert(filter->isAllowedUdsData(rawUds22, sizeof(rawUds22)) == true);
    assert(filter->isAllowedUdsData(rawUds35, sizeof(rawUds35)) == false);

    std::cout << "  SIDFilter test PASSED!" << std::endl;

    std::cout << "\nALL IPC UNIT TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
