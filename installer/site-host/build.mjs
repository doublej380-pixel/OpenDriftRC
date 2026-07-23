import {
  copyFileSync,
  cpSync,
  mkdirSync,
  rmSync,
} from "node:fs";

rmSync("dist", {
  recursive: true,
  force: true,
});

mkdirSync("dist/client", {
  recursive: true,
});

cpSync("public", "dist/client", {
  recursive: true,
});

copyFileSync(
  "public/installer.html",
  "dist/client/index.html",
);

mkdirSync("dist/server", {
  recursive: true,
});

copyFileSync(
  "server/index.js",
  "dist/server/index.js",
);

mkdirSync("dist/.openai", {
  recursive: true,
});

copyFileSync(
  ".openai/hosting.json",
  "dist/.openai/hosting.json",
);
