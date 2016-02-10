#ifndef CPP_BASE_UTIL_GRPC_MOCK_H_
#define CPP_BASE_UTIL_GRPC_MOCK_H_

#include <grpc/grpc.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>

namespace cpp_base {

#if 0

// An interface identical to GRPC's standard ClientReaderWriter.
// ClientReaderWriter is final and doesn't allow to derive from it
// so we have to replicate its interface like this.
template <class W, class R>
class MockClientReaderWriter {
  public:
    virtual ~MockClientReaderWriter() {}
    // Read() and Write() should return false only to signal stream closure.
    // If the stream is open but no message to read, Read() should block.
    virtual bool Read(R* msg) = 0;
    virtual bool Write(const W& msg) = 0;
    virtual bool WritesDone() { return true; }
    virtual grpc::Status Finish() { return grpc::Status::OK; }
};

// A standard way to test GRPC-involved code would be to mock the GRPC service,
// but GRPC's standard ClientReaderWriter class, which is the main tool to talk
// to an RPC service, is 'final', i.e., cannot be inherited from. The following
// class provides a workaround -- a unified interface to talk to either an
// actual GRPC service or a mock.
template <class W, class R>
class MockableGrpcStream {
  public:
    // These two makers wrap a MockableGrpcStream around the given stream.
    // They do not take ownership of the given pointers and they must remain
    // valid for the lifetime of this object.
    static std::unique_ptr<MockableGrpcStream<W, R>> MakeFromRealStream(
            grpc::ClientReaderWriter<W, R>* grpc_stream) {
        MockableGrpcStream<W, R>* s = new MockableGrpcStream(grpc_stream);
        return std::unique_ptr<MockableGrpcStream<W, R>>(s);
    }
    static std::unique_ptr<MockableGrpcStream<W, R>> MakeFromMockStream(
            MockClientReaderWriter<W, R>* mock_stream) {
        MockableGrpcStream<W, R>* s = new MockableGrpcStream(mock_stream);
        return std::unique_ptr<MockableGrpcStream<W, R>>(s);
    }

    bool Read(R* msg) {
        return (stream_ ? stream_->Read(msg) : mock_stream_->Read(msg));
    }
    bool Write(const W& msg) {
        return (stream_ ? stream_->Write(msg) : mock_stream_->Write(msg));
    }
    bool WritesDone() {
        return (stream_ ? stream_->WritesDone() : mock_stream_->WritesDone());
    }
    grpc::Status Finish() {
        return (stream_ ? stream_->Finish() : mock_stream_->Finish());
    }

  private:
    // These constructors do not take ownership of the given pointer.
    explicit MockableGrpcStream(grpc::ClientReaderWriter<W, R>* grpc_stream)
            : stream_(grpc_stream), mock_stream_(nullptr) { }
    explicit MockableGrpcStream(MockClientReaderWriter<W, R>* mock_stream)
            : stream_(nullptr), mock_stream_(mock_stream) { }

    // Only one of these two pointers is non-nullptr. mock_ is used only when
    // unit-testing. These pointers are 'borrowed'; ownership does not belong
    // to this class.
    grpc::ClientReaderWriter<W, R>* stream_;
    MockClientReaderWriter<W, R>* mock_stream_;
};

#endif

}  // namespace cpp_base

#endif  // CPP_BASE_UTIL_GRPC_MOCK_H_
