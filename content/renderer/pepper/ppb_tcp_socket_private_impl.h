// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PEPPER_PPB_TCP_SOCKET_PRIVATE_IMPL_H_
#define CONTENT_RENDERER_PEPPER_PPB_TCP_SOCKET_PRIVATE_IMPL_H_

#include <vector>

#include "base/compiler_specific.h"
#include "ppapi/shared_impl/private/tcp_socket_private_impl.h"

namespace content {

class PepperHelperImpl;

class PPB_TCPSocket_Private_Impl : public ::ppapi::TCPSocketPrivateImpl {
 public:
  static PP_Resource CreateResource(PP_Instance instance);
  static PP_Resource CreateConnectedSocket(
      PP_Instance instance,
      uint32 socket_id,
      const PP_NetAddress_Private& local_addr,
      const PP_NetAddress_Private& remote_addr);

  virtual void SendConnect(const std::string& host, uint16_t port) OVERRIDE;
  virtual void SendConnectWithNetAddress(
      const PP_NetAddress_Private& addr) OVERRIDE;
  virtual void SendSSLHandshake(
      const std::string& server_name,
      uint16_t server_port,
      const std::vector<std::vector<char> >& trusted_certs,
      const std::vector<std::vector<char> >& untrusted_certs) OVERRIDE;
  virtual void SendRead(int32_t bytes_to_read) OVERRIDE;
  virtual void SendWrite(const std::string& buffer) OVERRIDE;
  virtual void SendDisconnect() OVERRIDE;
  virtual void SendSetOption(PP_TCPSocket_Option name,
                             const ::ppapi::SocketOptionData& value) OVERRIDE;

 private:
  PPB_TCPSocket_Private_Impl(PP_Instance instance, uint32 socket_id);
  virtual ~PPB_TCPSocket_Private_Impl();

  static PepperHelperImpl* GetHelper(PP_Instance instance);

  DISALLOW_COPY_AND_ASSIGN(PPB_TCPSocket_Private_Impl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PEPPER_PPB_TCP_SOCKET_PRIVATE_IMPL_H_
