# Command Center (CMD)

The command center performs the following functionalities:

- Sets up the web front end.
- Maintains a list of registered back-end services.
- Classifies and forwards front-end requests to the appropriate services.
- Returns requested information to the front end.

Specifically, `app.py` starts the Flask server (listening to front end)
and web socket ASR router (listening to front end).

## Configuration

To configure the command center, please read the last section of [this](../../README.md).

## Major Dependencies

- [Flask](http://flask.pocoo.org/)
- [Apache Thrift](https://thrift.apache.org/)
- [scikit-learn](http://scikit-learn.org/stable/)
- [MongoDB](https://www.mongodb.com/)
- [Memcached](https://memcached.org/)

## Structure

- `apache/`: scripts and configuration files for the Apache HTTP server
- `controllers/`: web controllers, database module, Thrift modules, utilities module, 
query classification module, configuration file (`Config.py`), etc.
- `data/`: training data for query classifier
- `static/`: static contents of the web front end
- `templates/`: html Jinja templates
- `app.py`: top-level module
- `clear_db.sh`: script to clear MongoDB (only for testing)

## Build

```
make
```

This generates Thrift modules and thus only needs to be done once.

## Run

Start the server without using Apache:

```
make start_server
```

Open your browser and go to `http://localhost:3000/`. 
Notice that the command center assumes that the services specified in `controllers/Config.py` are running.
Make sure to start those services as well.
Read [the top level README](../../README.md) for more information about the configuration file.

We commend starting the Apache server in a Docker container (see [deploy](../../tools/deploy/)),
but you if you really want to use it on your host machine (which makes debugging hard),
select the configuration file in `apache/conf` (by default it is http, but if you need https,
you should modify `apache/conf/000-default_https.conf` and `apache/install_apache.sh`),
run `apache/install_apache.sh`, and follow the commands in [web-controller.yaml](../../tools/deploy/web-controller.yaml)
or [web-controller-https.yaml](../../tools/deploy/web-controller-https.yaml).
