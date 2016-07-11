struct QueryInput {
    // type of data (i.e. audio, image, etc)
    1: string type;

    // list of data (if sending multiple inputs) each a binary string
    2: list<string> data;

    // tags to pass information about data
    3: list<string> tags;
}

struct QuerySpec {
    1: string name;
    2: list<QueryInput> content;
}
