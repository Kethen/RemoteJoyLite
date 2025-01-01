set -xe
IMAGE_TAG="remotejoy_build"

if [ -n "$REBUILD_IMAGE" ]
then
	podman image rm -f $IMAGE_TAG
fi

if ! podman image exists $IMAGE_TAG
then
	podman image build -t $IMAGE_TAG -f Dockerfile
fi

podman run \
	--rm -it \
	--security-opt label=disable \
	-v ./:/workdir \
	-v ./script:/workdir/script:ro \
	-w /workdir \
	--entrypoint /bin/bash \
	$IMAGE_TAG \
	script
