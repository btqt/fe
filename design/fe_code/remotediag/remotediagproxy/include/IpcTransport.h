#ifndef RDG_IPC_TRANSPORT_H
#define RDG_IPC_TRANSPORT_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace rdgipc {

/**
 * @brief Transport layer interface for IPC communication
 * 
 * This interface handles low-level socket operations and frame transmission.
 * It is stable and does not need changes when adding new commands/messages.
 */
class IpcTransport {
public:
    /**
     * @brief Message frame structure
     */
    struct Frame {
        uint32_t messageType;  // Request, Response, or Callback
        uint32_t messageId;    // Command ID or Callback ID
        uint32_t correlationId; // Request/response correlation token, 0 for callbacks
        std::vector<uint8_t> payload;
    };

    /** Maximum frame payload size; receivers drop the connection above this.
     *  Must exceed the largest upload file (4 MB) plus encryption/framing overhead. */
    static constexpr size_t kMaxPayloadSize{8U * 1024U * 1024U};

    /**
     * @brief Callback type for received frames
     */
    using FrameReceivedCallback = std::function<void(const Frame &frame)>;

    virtual ~IpcTransport() = default;

    /**
     * @brief Start the transport (connect or listen)
     * @return true if successful
     */
    virtual bool start() noexcept = 0;

    /**
     * @brief Stop the transport
     */
    virtual void stop() noexcept = 0;

    /**
     * @brief Send a frame
     * @param frame Frame to send
     * @return true if successful
     */
    virtual bool sendFrame(const Frame &frame) noexcept = 0;

    /**
     * @brief Set callback for received frames
     * @param callback Callback function
     */
    virtual void setFrameReceivedCallback(FrameReceivedCallback callback) noexcept = 0;

    /**
     * @brief Check if transport is connected
     * @return true if connected
     */
    virtual bool isConnected() const noexcept = 0;

    /**
     * @brief Block until a peer connection is established
     * @param timeoutMs Maximum wait in milliseconds
     * @return true if connected before the timeout
     */
    virtual bool waitUntilConnected(uint32_t timeoutMs) noexcept = 0;
};

} // namespace rdgipc

#endif // RDG_IPC_TRANSPORT_H
