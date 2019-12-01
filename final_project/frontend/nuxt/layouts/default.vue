<template>
  <div>
    <Navbar />
    <nuxt />
  </div>
</template>

<script lang="ts">
import Vue from 'vue'
import Navbar from '~/components/Navbar.vue'
export default Vue.extend({
  name: 'Main',
  components: {
    Navbar
  }
})
</script>

<style lang="scss">
@import '~/assets/styles/global.scss';
</style>
