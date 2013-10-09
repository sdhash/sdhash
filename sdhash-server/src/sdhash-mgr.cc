/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

#include "sdhashsrv.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace sdhash;

using namespace boost;

int main(int argc, char** argv) {
  boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  sdhashsrvClient client(protocol);

  if (argc == 2) {
    if (!strncmp(argv[1],"stop",3)) {
      try {
        transport->open();
        client.ping();
        client.shutdown();
        cout << "stop signal sent to sdhash-srv" << endl;
        transport->close();
      } catch (TException &tx) {
        cerr << "Cannot connect "<< tx.what() << endl;
      }
    } else if (!strncmp(argv[1],"start",5)) {
        // start sdhash-srv
        cout << "starting sdhash-srv with defaults" << endl;
        execl("sdhash-srv","sdhash-srv",NULL);
    } else if (!strncmp(argv[1],"status",6)) {
      try {
        transport->open();
        client.ping();
        cout << "sdhash-srv is responding to requests" << endl;
        transport->close();
      } catch (TException &tx) {
        cerr << "Cannot connect, server not online." << endl;
      }
    }
  } else {
    cerr << "Usage: " << argv[0] << " start|stop|status" << endl;
  }
}
