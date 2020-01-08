Testing Docker Image
====================

Local testing of the [Docker image](Dockerfile) used by the [Jenkinsfile](Jenkinsfile) can be
performed by running the following in the project root directory:

```shell
$ name=$(basename $(pwd)) work_dir=/home/user/$name
$ docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -f tools/ci/Dockerfile -t $name .
$ docker run -it --volume $(pwd):$work_dir --workdir=$work_dir $name
```
