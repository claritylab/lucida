# Template for building your own microservice

This is a template for a user to refer to when creating their own microservice for Lucida. It includes 3 versions of microservice implementation (c++, Java, python). You can choose your favorite language and implement your own service based on the template. We use both Apache Thrift and Facebook Thrift as our Lucida RPC framework. Thrift is an RPC framework with the advantages of being efficient and language-neutral. It was originally developed by Facebook and now developed by both the open-source community (Apache Thrift) and Facebook.

## Major Dependencies

- [Facebook Thrift](https://github.com/facebook/fbthrift)
- [gradle](https://gradle.org/)

# Structure

- [`cpp`](cpp): implementation of C++ template 
- [`java`](java): implementation for Java template
- [`python`](python): implementation for Python template