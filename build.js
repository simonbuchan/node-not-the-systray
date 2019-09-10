const os = require('os');
const fs = require('fs');
const { promisify } = require('util');
const node_gyp = require('node-gyp');

// stripped down version of node-gyp/bin/node-gyp.js
// runs main node-gyp logic, but copies .node output to root

function parseGyp(path) {
    return JSON.parse(fs.readFileSync(path, 'utf8').replace(/\#.+\n/, ''))
}

async function main() {
    const gyp = node_gyp();
    // reuse existing .node-gyp files
    gyp.devDir = `${os.homedir()}/.node-gyp`;
    gyp.parseArgv(process.argv);

    // Override default target version.
    if (!gyp.opts.target) {
        gyp.opts.target = 'v10.16.0';
    }

    for (const { name, args } of gyp.todo) {
        await promisify(gyp.commands[name])(args);
        if (name == "build") {
            const binding = parseGyp(`${__dirname}/binding.gyp`);
            // Compute build output dir the same way as node-gyp/lib/build.js
            const config = parseGyp(`${__dirname}/build/config.gypi`);
            let buildType = config.target_defaults.default_configuration;
            if ('debug' in gyp.opts) {
                buildType = gyp.opts.debug ? 'Debug' : 'Release';
            }
            if (!buildType) {
                buildType = 'Release';
            }
            // Assume TargetPath is the default: build\$buildType\$target_name.node
            const name = `${binding.targets[0].target_name}.node`;
            const buildPath = `${__dirname}/build/${buildType}/${name}`;
            const outPath = `${__dirname}/${name}`;
            console.log('Copying %O -> %O', buildPath, outPath);
            fs.copyFileSync(buildPath, outPath);
        }
    }
}

main().catch((error) => {
    console.error(error);
    process.exit(1);
});