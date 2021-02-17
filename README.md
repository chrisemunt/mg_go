# mg\_go

A GO Extension for InterSystems **Cache/IRIS** and **YottaDB**.

Chris Munt <cmunt@mgateway.com>  
17 February 2021, M/Gateway Developments Ltd [http://www.mgateway.com](http://www.mgateway.com)


* Current Release: Version: 1.2; Revision 3.
* Two connectivity models to the InterSystems or YottaDB database are provided: High performance via the local database API or network based.
* [Release Notes](#RelNotes) can be found at the end of this document.

Contents

* [Overview](#Overview")
* [Pre-requisites](#PreReq")
* [Installing mg\_go](#Install)
* [Connecting to the database](#Connect)
* [Invocation of database commands](#DBCommands)
* [Invocation of database functions](#DBFunctions)
* [Transaction Processing](#TProcessing)
* [Direct access to InterSystems classes (IRIS and Cache)](#DBClasses)
* [License](#License)


## <a name="Overview"></a> Overview 

**mg\_go** is an Open Source GO extension developed for InterSystems **Cache/IRIS** and the **YottaDB** database.  It will also work with the **GT.M** database.

The **mg\_go** extension connects to these databases using their high performance C-based APIs. There is also the option of connecting to the database over the network.

## <a name="PreReq"></a> Pre-requisites

**Go** installation:

       https://golang.org/

InterSystems **Cache/IRIS** or **YottaDB** (or similar M database):

       https://www.intersystems.com/
       https://yottadb.com/

## <a name="Install"></a> Installing mg\_go

Install the core database interface module (**mg\_dba.so** for UNIX and **mg\_dba.dll** for Windows) in a directory of your choosing.

The **mg\_go** extension is a module written in **Go** and this is included in your Go project.  **mg\_go** dynamically loads the **mg\_dba** library (written in C) and this latter module is responsible for connecting **mg\_go** to the database either via the database's API or over the network.

### Building the mg_dba module from the source code provided

UNIX (in the /src/ directory):

       make

Windows (in the /src/ directory):

       nmake -f Makefile.win

### Installing the mg\_go extension

Install the GO extension (essentially a GO package) in your GO source directory.

       .../go/src/mg_go/

In this directory you will find **mg.go.unix** and **mg.go.windows**.  Rename the appropriate one for your OS as **mg.go**

### Installing DB Superserver

The DB Superserver is required for:

* Network based access to databases.

Two M routines need to be installed (%zmgsi and %zmgsis).  These can be found in the *Service Integration Gateway* (**mgsi**) GitHub source code repository ([https://github.com/chrisemunt/mgsi](https://github.com/chrisemunt/mgsi)).  Note that it is not necessary to install the whole *Service Integration Gateway*, just the two M routines held in that repository.

#### Installation for InterSystems Cache/IRIS

Log in to the %SYS Namespace and install the **zmgsi** routines held in **/isc/zmgsi\_isc.ro**.

       do $system.OBJ.Load("/isc/zmgsi_isc.ro","ck")

Change to your development UCI and check the installation:

       do ^%zmgsi

       M/Gateway Developments Ltd - Service Integration Gateway
       Version: 4.0; Revision 16 (11 February 2021)


#### Installation for YottaDB

The instructions given here assume a standard 'out of the box' installation of **YottaDB** (version 1.30) deployed in the following location:

       /usr/local/lib/yottadb/r130

The primary default location for routines:

       /root/.yottadb/r1.30_x86_64/r

Copy all the routines (i.e. all files with an 'm' extension) held in the GitHub **/yottadb** directory to:

       /root/.yottadb/r1.30_x86_64/r

Change directory to the following location and start a **YottaDB** command shell:

       cd /usr/local/lib/yottadb/r130
       ./ydb

Link all the **zmgsi** routines and check the installation:

       do ylink^%zmgsi

       do ^%zmgsi

       M/Gateway Developments Ltd - Service Integration Gateway
       Version: 4.0; Revision 16 (11 February 2021)

Note that the version of **zmgsi** is successfully displayed.


### Starting the DB Superserver

The default TCP server port for **zmgsi** is **7041**.  If you wish to use an alternative port then modify the following instructions accordingly.

* For InterSystems DB servers the concurrent TCP service should be started in the **%SYS** Namespace.

Start the DB Superserver using the following command:

       do start^%zmgsi(0) 

To use a server TCP port other than 7041, specify it in the start-up command (as opposed to using zero to indicate the default port of 7041).

* For YottaDB, as an alternative to starting the DB Superserver from the command prompt, Superserver processes can be started via the **xinetd** daemon.  Instructions for configuring this option can be found in the **mgsi** repository [here](https://github.com/chrisemunt/mgsi)

GO code using the **mg\_go** functions will, by default, expect the database server to be listening on port **7041** of the local server (localhost).  However, **mg\_go** provides the functionality to modify these default settings at run-time.  It is not necessary for the GO installation to reside on the same host as the database server.


## <a name="Connect"></a> Connecting to the database

### Including the mg_go package in your project

To use the **mg\_go** extension you should include it in the list of packages required for your project.  For a  very basic GO project this might look something like:

       import (
          "fmt"
          "mg_go"
       )


### Open a connection to the database (API based connectivity)

In the following examples, modify all paths (and any user names and passwords) to match those of your own installation.


#### InterSystems Cache

Assuming Cache is installed under **/opt/cache20181/**

       db := mg_go.New("Cache")
           db.APImodule = "../bin/mg_dba.so" // this will be mg_dba.dll for Windows
           db.Path = "/opt/cache20181/mgr"
           db.Username = "_SYSTEM"
           db.Password = "SYS"
           db.Namespace = "USER"
       result := db.Open()


#### InterSystems IRIS

Assuming IRIS is installed under **/opt/IRIS20181/**

       db := mg_go.New("IRIS")
           db.APImodule = "../bin/mg_dba.so" // this will be mg_dba.dll for Windows
           db.Path = "/opt/IRIS20181/mgr"
           db.Username = "_SYSTEM"
           db.Password = "SYS"
           db.Namespace = "USER"
       result := db.Open()


#### YottaDB

Assuming an 'out of the box' YottaDB installation under **/usr/local/lib/yottadb/r130**.

       db := mg_go.New("YottaDB")
           db.APImodule = "../bin/mg_dba.so"
           db.Path = "/usr/local/lib/yottadb/r130"
           db.EnvVars = db.EnvVars + "ydb_dir=/root/.yottadb\n"
           db.EnvVars = db.EnvVars + "ydb_rel=r1.30_x86_64\n"
           db.EnvVars = db.EnvVars + "ydb_gbldir=/root/.yottadb/r1.30_x86_64/g/yottadb.gld\n"
           db.EnvVars = db.EnvVars + "ydb_routines=/root/.yottadb/r1.30_x86_64/o*(/root/.yottadb/r1.30_x86_64/r /root/.yottadb/r) /usr/local/lib/yottadb/r130/libyottadbutil.so\n"
           db.EnvVars = db.EnvVars + "ydb_ci=/usr/local/lib/yottadb/r130/cm.ci\n"
           db.EnvVars = db.EnvVars + "\n"
       result := db.Open()


### Open a connection to the database (Network based connectivity)

Assuming the server (**Cache** in this example) is listening on port **7041** on host **localhost**

       db := mg_go.New("Cache")
           db.APImodule = "../bin/mg_dba.so" // this will be mg_dba.dll for Windows
           db.Host = "localhost"
           db.TCPPort = 7041
           db.Username = "_SYSTEM"
           db.Password = "SYS"
           db.Namespace = "USER"
       result := db.Open()


#### Connecting to the database via the MGWSI Service Integration Gateway

If the M/Gateway Service Integration Gateway (**MGWSI**) is available, **mg\_go** can connect to the database via this facility.

Assuming the **MGWSI** Gateway is listening on port **7040** on host **localhost** and the target Server (**Cache** in this example) is named as **LOCAL** in the Service integration Gateway configuration.

       db := mg_go.New("Cache")
           db.APImodule = "../bin/mg_dba.so" // this will be mg_dba.dll for Windows
           db.Host = "localhost"
           db.TCPPort = 7040
           db.Server = "LOCAL"
           db.Username = "_SYSTEM"
           db.Password = "SYS"
           db.Namespace = "USER"
       result := db.Open()

### Additional database properties that can be set before opening a connection

### Setting the DB Server Response Timeout

       db.Timeout := <time in seconds>

This setting applies to network based connectivity to the DB Server.  The default value is 30 seconds.

Example: set timeout to 60 seconds:

       db.Timeout := 60


### Setting the DB Server input buffer size

       db.InputBufferSize = <size in Bytes>

The buffer size must be large enough to hold the maximum size of the request or response data from the DB Server.  The default value is 32767 Bytes.  However for newer InterSystems databases the maximum string size can be up to 3,641,144 Bytes.

Example: set the buffer size to the maximum allowed for InterSytems databases:

       db.Timeout := 3641144


### Return the version of mg\_go

       version := db.Version()

Example:

       fmt.Printf("\nVersion of mg\_go: %s\n",  db.Version())


### Get current Namespace

      namespace := db.GetNamespace()

Example:

      namespace := db.GetNamespace()
      fmt.Printf("\nCurrent Namespace ns=%v\n", namespace)


### Change current Namespace

      result := db.SetNamespace(<namespace>)

Example:

      result := db.SetNamespace("USER")


### Close database connection

       db.Close()


## <a name="DBCommands"></a> Invocation of database commands

### Register a global name

       global := db.Global(<global_name>)

Example (using a global named "Person"):

       person := db.Global("Person")

### Set a record

       result := <global>.Set(<key>, <data>)
      
Example:

       person.Set(1, "John Smith")

### Get a record

       result := <global>.Get(<key>)
      
Example:

       result := person.Get(1);
       fmt.Printf("\nName :  %s\n", result.Data.(string))

### Delete a record

       result := <global>.Delete(<key>)
      
Example:

       result := person.Delete(1)


### Check whether a record is defined

       result := <global>.Defined(<key>)
      
Example:

       result := person.Defined(1)


### Parse a set of records (in order)

       result := <global>.Next(<key>)
      
Example:

       id := ""
       for r := person.Next(id); r.OK; r = person.Next(id) {
          id = r.Data.(string)
          fmt.Printf("\nPerson ID: %s, Name: %s", id, person.Get(id).Data.(string))
        }


### Parse a set of records (in reverse order)

       result := <global>.Previous(<key>)

      
Example:

       id = ""
       for r := person.Previous(id); r.OK; r = person.Previous(id) {
       id = r.Data.(string)
          fmt.Printf("\nPerson ID: %s, Name: %s", id, person.Get(id).Data.(string))
       }

### Increment the value held in a global node

       result := <global>.Increment(<key>)
      
Example (increment the ^Person global by 1 and return the next value):

       result := person.Increment(1)


## <a name="DBFunctions"> Invocation of database functions

       result := db.Function(<function>, <arguments>)
      
Example:

M routine called 'math':

       add(a, b) ; Add two numbers together
                 quit (a+b)

Go invocation:

      result := db.Function("add^math", 2, 3)
      fmt.Printf("\nFunction result: %v\n", fr)


## <a name="TProcessing"></a> Transaction Processing

M DB Servers implement Transaction Processing by means of the methods described in this section.

* With YottaDB, these methods are only available over network based connectivity to the DB Server.

### Start a Transaction

       result := db.TStart()

* On successful completion this method will return zero, or an error code on failure.

Example:

       result := db.TStart()


### Determine the Transaction Level

       tlevel := db.TLevel()

* Transactions can be nested and this method will return the level of nesting.  If no Transaction is active this method will return zero.  Otherwise a positive integer will be returned to represent the current depth of Transaction nesting.

Example:

       result := db.TLevel()


### Commit a Transaction

       result := db.TCommit()

* On successful completion this method will return zero, or an error code on failure.

Example:

       result := db.TCommit()


### Rollback a Transaction

       result := db.TRollback()

* On successful completion this method will return zero, or an error code on failure.

Example:

       result := db.TRollback()


## <a name="DBClasses"> Direct access to InterSystems classes (IRIS and Cache)

To illustrate these methods, the following simple class will be used:

      Class User.customer Extends %Persistent
      {
         Property number As %Integer;
         Property name As %String;
         ClassMethod MyClassMethod(x As %Integer) As %Integer
         {
            // do some work
            Quit result
         }
         Method MyMethod(x As %Integer) As %Integer
         {
            // do some work
            Quit result
         }
      }

### Register a Cache/IRIS class

      class := db.Class(<class_name>)

Example:

      customer := db.Class("User.customer")

### Invoke a ClassMethod

     result := <class>.ClassMethod(<classmethod_name>, <arguments>)

Example:

      result := customer.ClassMethod("MyClassMethod", 3)

### Open a specific instance of a Class

Example (using instance/record #1):

     result := customer.ClassMethod("%OpenId", "1")

### Get a property

     result := <class>.GetProperty(<property_name>)

Example:

     result = customer.GetProperty("name")
     fmt.Printf("\nCustomer name: %s\n", result.Data.(string))

      result := customer.ClassMethod("MyClassMethod", 3)

### Set a property

     result := <class>.SetProperty(<property_name>, <value>)

Example:

     result = customer.SetProperty("name", "John Smith")


### Invoke a Method

     result := <class>.Method(<method_name>, <arguments>)

Example:

      result := customer.ClassMethod("MyMethod", 3)


## <a name="License"></a> License

Copyright (c) 2018-2021 M/Gateway Developments Ltd,
Surrey UK.                                                      
All rights reserved.
 
http://www.mgateway.com                                                  
Email: cmunt@mgateway.com
 
 
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.      


## <a name="RelNotes"></a>Release Notes

### v1.0.1 (4 July 2019)

* Initial Release

### v1.1.2 (9 January 2020)

* Indroduce the option of connecting to the M server over the network.

### v1.1.2a (11 January 2021)

* Verify that **mg\_go** works with the latest version of Go: 1.15.6
* Restructure the documentation.

### v1.2.3 (17 February 2021)

* Introduce support for M transaction processing: tstart, $tlevel, tcommit, trollback.
* Allow the DB server response timeout to be modified via the db.Timeout property.
	* db.Timeout = [timeout]
* Allow the input buffer size to be modified via the db.InputBufferSize property.
	* db.InputBufferSize = [size]
* Improved exception handling for DB connectivity errors.