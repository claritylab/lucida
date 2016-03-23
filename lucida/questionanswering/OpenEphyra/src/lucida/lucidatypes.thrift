struct QueryInput {
    // type of data (i.e. audio, image, etc)
    1: string type;

    // list of data (if sending multiple inputs) each a binary string
    2: list<string> data;

    // tags to pass information about data
    3: optional list<string> tags;
}

// This QuerySpec was used to define the type of query for Sirius (VIQ, VQ) so
// the list included audio and image for example. I don't think we need this.
// We should use QueryInput as the main payload to shuttle data around
struct QuerySpec {
    1: optional string name;
    2: list<QueryInput> content;
}
