module.exports = {
  testEnvironment: 'jsdom', // 使用 jsdom 模拟浏览器环境
  transform: {
    '^.+\\.jsx?$': 'babel-jest', // 使用 babel-jest 转换 JavaScript 文件
  },
  moduleFileExtensions: ['js', 'jsx'], // 支持的文件扩展名
};