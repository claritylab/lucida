Generic Backend Service
=======================

## Introduction
This backend offers a Thrift and REST communication and can be built via Gradle. It can be used as boilerplate to
create microservices for components in the IBM Sapphire Project. Communication aspects are bundled in communication,
whereas the actual implementation should be called from the orchestration package. Feel free to add additional
packages.

## How to Adapt Generic Backend
To build a new component from this template:
1) Refactor Java package in src/main/java to new package name, e.g.
   edu.umich.sapphire.genericbackend to edu.umich.sapphire.faqclassifier
2) Change name in settings.gradle accordingly, check all package reference in build.gradle are adjusted
3) If using IntelliJ, go into File -> Project Structure and rename entries under Project & Modules
4) Under Run -> Edit Configurations change Name and Main accordingly
5) Adapt path in edu.umich.sapphire.genericbackend.communication.RestHandler
6) Replace this README with one describing the new backend, but do not remove license and contributors parts
   (last two paragraphs)
   
Added generic backend as follows: in `/lucida/commandcenter/controllers/Config.py` changed QA service line to `'QA' : Service('QA', 9092, 'text', 'text'),` and QA classifier description to `'text' : { 'class_QA' :  Graph([Node('QA')]) },`

## Guidelines
- Follow test-driven development and microservice architecture
- Support REST and Thrift as means of communication
- Allow for Docker / Kubernetes deployments
- Consider the 12 Factors, cmp. http://12factor.net/

## Contributors
| Name        | Main Contributions          |
|-------------|-----------------------------|
| k0105       | REST, Thrift, Gradle, Tests |

### ASLv2
    Generic Backend Service
    Copyright 2016 RWTH Aachen, University of Michigan

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
