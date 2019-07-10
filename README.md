# mg_go

A GO Extension for InterSystems **Cache/IRIS** and **YottaDB**.

Chris Munt <cmunt@mgateway.com>  
4 July 2019, M/Gateway Developments Ltd [http://www.mgateway.com](http://www.mgateway.com)


* Current Release: Version: 1.0; Revision 1 (10 July 2019)

## Overview

**mg_go** is an Open Source GO extension developed for InterSystems **Cache/IRIS** and the **YottaDB** database.  It will also work with the **GT.M**.

The **mg_go** extension connects to these databases using their high performance C-based APIs. 

## Pre-requisites

GO installation:

       https://golang.org/

InterSystems **Cache/IRIS** or **YottaDB** (or similar M database):

       https://www.intersystems.com/
       https://yottadb.com/

## Installing mg_go

Install the core database interface module (**mg_dba.so** for UNIX and **mg_dba.dll** for Windows) in a directory of your choosing.

Install the GO extension (essentially a GO package) in your GO source directory.

       .../go/src/mg_go/

In this directory you will find **mg.go.unix** and **mg.go.windows**.  Rename the appropriate one for your OS as **mg.go**

## Documentation

### Including the mg_go package in your project

To use the **mg_go** extension you should include it in the list of packages required for your project.  For a  very basic GO project this might look something like:

       import (
          "fmt"
          "mg_go"
       )

### Open a connection to the database

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

Assuming an 'out of the box' YottaDB installation under **/usr/local/lib/yottadb/r122**.

       db := mg_go.New("YottaDB")
           db.APImodule = "../bin/mg_dba.so"
           db.Path = "/usr/local/lib/yottadb/r122"
           db.EnvVars = db.EnvVars + "ydb_dir=/root/.yottadb\n"
           db.EnvVars = db.EnvVars + "ydb_rel=r1.22_x86_64\n"
           db.EnvVars = db.EnvVars + "ydb_gbldir=/root/.yottadb/r1.22_x86_64/g/yottadb.gld\n"
           db.EnvVars = db.EnvVars + "ydb_routines=/root/.yottadb/r1.22_x86_64/o*(/root/.yottadb/r1.22_x86_64/r /root/.yottadb/r) /usr/local/lib/yottadb/r122/libyottadbutil.so\n"
           db.EnvVars = db.EnvVars + "ydb_ci=/usr/local/lib/yottadb/r122/cm.ci\n"
           db.EnvVars = db.EnvVars + "\n"
       result := db.Open()

### Invocation of database commands

#### Register a global name

       global := db.Global(<global_name>)

Example (using a global named "Person"):

       person := db.Global("Person")

#### Set a record

       result := <global>.Set(<key>, <data>)
      
Example:

       person.Set(1, "John Smith")

#### Get a record

       result := <global>.Get(<key>)
      
Example:

       result := person.Get(1);
       fmt.Printf("\nName :  %s\n", result.Data.(string))

#### Delete a record

       result := <global>.Delete(<key>)
      
Example:

       result := person.Delete(1)


#### Check whether a record is defined

       result := <global>.Defined(<key>)
      
Example:

       result := person.Defined(1)


#### Parse a set of records (in order)

       result := <global>.Next(<key>)
      
Example:

       id := ""
       for r := person.Next(id); r.OK; r = person.Next(id) {
          id = r.Data.(string)
          fmt.Printf("\nPerson ID: %s, Name: %s", id, person.Get(id).Data.(string))
        }


#### Parse a set of records (in reverse order)

       result := <global>.Previous(<key>)

      
Example:

       id = ""
       for r := person.Previous(id); r.OK; r = person.Previous(id) {
       id = r.Data.(string)
          fmt.Printf("\nPerson ID: %s, Name: %s", id, person.Get(id).Data.(string))
       }

### Invocation of database functions

       result := db.Function(<function>, <arguments>)
      
Example:

M routine called 'math':

       add(a, b) ; Add two numbers together
                 quit (a+b)

Go invocation:

      result := db.Function("add^math", 2, 3)
      fmt.Printf("\nFunction result: %v\n", fr)


### Methods only available with InterSystems Cache and IRIS

#### Get current Namespace

      namespace := db.GetNamespace()

Example:

      namespace := db.GetNamespace()
      fmt.Printf("\nCurrent Namespace ns=%v\n", namespace)

#### Change current Namespace

      result := db.SetNamespace(<namespace>)

Example:

      result := db.SetNamespace("USER")

#### Access to Cache/IRIS Classes

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

##### Register a Cache/IRIS class

      class := db.Class(<class_name>)

Example:

      customer := db.Class("User.customer")

##### Invoke a ClassMethod

     result := <class>.ClassMethod(<classmethod_name>, <arguments>)

Example:

      result := customer.ClassMethod("MyClassMethod", 3)

##### Open a specific instance of a Class

Example (using instance/record #1):

     result := customer.ClassMethod("%OpenId", "1")

##### Get a property

     result := <class>.GetProperty(<property_name>)

Example:

     result = customer.GetProperty("name")
     fmt.Printf("\nCustomer name: %s\n", result.Data.(string))

      result := customer.ClassMethod("MyClassMethod", 3)

##### Set a property

     result := <class>.SetProperty(<property_name>, <value>)

Example:

     result = customer.SetProperty("name", "John Smith")


##### Invoke a Method

     result := <class>.Method(<method_name>, <arguments>)

Example:

      result := customer.ClassMethod("MyMethod", 3)


### Return the version of mg_go

       version := db.Version()

Example:

       fmt.Printf("\nVersion of mg_go: %s\n",  db.Version())


### Close database connection

       db.Close()

## License

Copyright (c) 2018-2019 M/Gateway Developments Ltd,
Surrey UK.                                                      
All rights reserved.
 
http://www.mgateway.com                                                  
Email: cmunt@mgateway.com
 
 
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.      

