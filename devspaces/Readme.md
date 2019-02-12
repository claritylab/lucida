# Development with Devspaces

### Devspaces 

Manage your Devspaces https://www.devspaces.io/.

Read up-to-date documentation about cli installation and operation in https://www.devspaces.io/devspaces/help.

Here follows the main commands used in Devspaces cli.

|action   |Description                                                                                   |
|---------|----------------------------------------------------------------------------------------------|
|`devspaces --help`                    |Check the available command names.                               |
|`devspaces create [options]`          |Creates a DevSpace using your local DevSpaces configuration file |
|`devspaces start <devSpace>`          |Starts the DevSpace named \[devSpace\]                           |
|`devspaces bind <devSpace>`           |Syncs the DevSpace with the current directory                    |
|`devspaces info <devSpace> [options]` |Displays configuration info about the DevSpace.                  |
|`devspaces ls`                        |Displays status of all devspaces                                 |

Use `devspaces --help` to know about updated commands.

#### Development flow

You should have Devspaces cli services started and logged to develop with Devspaces.
The following commands should be issued from **project directory**.

1 - Create Devspaces

    - Go to https://devspaces.io/devspaces/list , click `Add Devspace`
    - Select Create from a public Docker image
    - Search for `lucida` and select the repository. You need to have devspaces running on your local machine for this to work.
    - For devspace name enter `df-lucida`
    - For docker Image search for `devspaces/lucida`
    - For ports select 3000 and 27017 with HTTP
    - Don't add any ENV variables
    - Wait for validation

2 - Start containers

```bash
devspaces start df-lucida
```


3 - Grab some container info

```bash
devspaces info df-lucida
```

4 - Connect to development container

```bash
devspaces exec df-lucida
```

5 - Build 

```bash
cd /data/lucida && make local
```
6 - Start

```bash
cd /data/lucida && make start_all
```
