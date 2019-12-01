require('dotenv').config()
const apiurl = process.env.APIURL

const name = 'emergency services'

module.exports = {
  mode: 'spa',

  globalName: name,

  env: {
    apiurl
  },

  /*
   ** Headers of the page
   */
  head: {
    title: process.env.npm_package_name || '',
    meta: [
      { charset: 'utf-8' },
      { name: 'viewport', content: 'width=device-width, initial-scale=1' },
      {
        hid: 'description',
        name: 'description',
        content: process.env.npm_package_description || ''
      }
    ],
    link: [{ rel: 'icon', type: 'image/x-icon', href: '/favicon.ico' }]
  },
  /*
   ** Customize the progress-bar color
   */
  loading: { color: '#fff' },

  /*
   ** Global CSS
   */
  css: [],
  /*
   ** Plugins to load before mounting the App
   */
  plugins: [
    // { src: '~/plugins/chat', ssr: false },
    { src: '~/plugins/vueselect', ssr: false },
    { src: '~/plugins/toast', ssr: false }
  ],
  /*
   ** Nuxt.js dev-modules
   */
  buildModules: [
    // Doc: https://github.com/nuxt-community/eslint-module
    '@nuxtjs/eslint-module',
    '@nuxt/typescript-build'
  ],
  /*
   ** Nuxt.js modules
   */
  modules: [
    // Doc: https://bootstrap-vue.js.org
    // 'bootstrap-vue/nuxt',
    // Doc: https://axios.nuxtjs.org/usage
    '@nuxtjs/style-resources',
    '@nuxtjs/axios',
    '@nuxtjs/pwa',
    // Doc: https://github.com/nuxt-community/dotenv-module
    '@nuxtjs/dotenv'
  ],

  /*
   ** Sitemap config
   */
  sitemap: {
    hostname: 'localhost',
    path: '/sitemap-main.xml',
    gzip: false,
    exclude: ['/admin', '/admin/**'],
    defaults: {
      changefreq: 'daily',
      priority: 1,
      lastmod: new Date(),
      lastmodrealtime: true
    }
  },

  /*
   ** generate config
   */
  generate: {
    fallback: '404.html'
  },

  /*
   ** scss global config
   */
  styleResources: {},

  /**
   * pwa options
   */
  pwa: {
    icon: {
      targetDir: 'i'
    }
  },
  /*
   ** Axios module configuration
   ** See https://axios.nuxtjs.org/options
   */
  axios: {
    baseURL: apiurl
  },
  /*
   ** Build configuration
   */
  build: {
    /*
     ** You can extend webpack config here
     */
    filenames: {
      app: ({ isDev }) => (isDev ? '[name].js' : '[chunkhash:5].js'),
      chunk: ({ isDev }) => (isDev ? '[name].js' : '[chunkhash:5].js'),
      css: ({ isDev }) => (isDev ? '[name].css' : '[contenthash:5].css'),
      img: ({ isDev }) => (isDev ? '[path][name].[ext]' : 'img/[hash:5].[ext]'),
      font: ({ isDev }) =>
        isDev ? '[path][name].[ext]' : 'fonts/[hash:5].[ext]',
      video: ({ isDev }) =>
        isDev ? '[path][name].[ext]' : 'videos/[hash:5].[ext]'
    },
    extend(config, ctx) {}
  }
}
