const vscode = require("vscode");
const builtins = require("./builtins.json");

const keywordList = [
  "if",
  "else",
  "while",
  "do",
  "for",
  "foreach",
  "switch",
  "case",
  "default",
  "break",
  "continue",
  "return",
  "function",
  "global",
  "unset",
  "include",
  "include_once",
  "as"
];

const literalList = [
  "true",
  "false",
  "null",
  "undefined",
  "void"
];

function buildFunctionItem(name) {
  const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
  item.insertText = new vscode.SnippetString(`${name}($0)`);
  return item;
}

function buildKeywordItem(name) {
  return new vscode.CompletionItem(name, vscode.CompletionItemKind.Keyword);
}

function buildLiteralItem(name) {
  return new vscode.CompletionItem(name, vscode.CompletionItemKind.Constant);
}

function activate(context) {
  const provider = vscode.languages.registerCompletionItemProvider(
    "lx",
    {
      provideCompletionItems() {
        const items = [];
        for (const name of builtins) items.push(buildFunctionItem(name));
        for (const name of keywordList) items.push(buildKeywordItem(name));
        for (const name of literalList) items.push(buildLiteralItem(name));
        return items;
      }
    }
  );

  context.subscriptions.push(provider);
}

function deactivate() {}

module.exports = {
  activate,
  deactivate
};
