struct QueryInput {
    // type of data
    1: string type;

    // list of data
    2: list<string> data;

    // tags to pass information about data
    3: list<string> tags;
}

struct QuerySpec {
    1: string name;
    2: list<QueryInput> content;
}
