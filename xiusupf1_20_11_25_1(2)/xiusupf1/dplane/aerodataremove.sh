#!/bin/bash


namespace="test"
upfset="dupf1"
componentupf="upf"


if [ $name == $componentamf ]

[ $name == $componentupf ]; then
    aql_output=$(aql -c "truncate $namespace.$upfset")
        echo "$aql_output"
else
    echo "No Set Data to Clear $name"
fi

