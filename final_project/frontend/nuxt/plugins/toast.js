import Vue from 'vue'
import Toasted from 'vue-toasted'
import { toasts } from '~/assets/config'

Vue.use(Toasted, {
  iconPack: 'fontawesome'
})

const erroroptions = {
  type: 'error',
  icon: 'search',
  action: {
    text: 'Close',
    onClick: (e, toastObject) => {
      toastObject.goAway(0)
    }
  }
}

for (const key in toasts) {
  erroroptions[key] = toasts[key]
}

Vue.toasted.register(
  'info',
  (payload) => {
    if (!payload.message) {
      return 'info'
    }
    return payload.message
  },
  erroroptions
)

Vue.toasted.register(
  'error',
  (payload) => {
    if (!payload.message) {
      return 'Oops.. Something Went Wrong..'
    }
    return payload.message
  },
  erroroptions
)

const successoptions = {
  type: 'success',
  icon: 'search',
  action: {
    text: 'Close',
    onClick: (e, toastObject) => {
      toastObject.goAway(0)
    }
  }
}

for (const key in toasts) {
  successoptions[key] = toasts[key]
}

Vue.toasted.register(
  'success',
  (payload) => {
    if (!payload.message) {
      return 'Success!'
    }
    return payload.message
  },
  successoptions
)
