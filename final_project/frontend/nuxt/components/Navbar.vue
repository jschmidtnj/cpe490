<template>
  <header class="navbar">
    <section class="navbar-section">
      <img
        src="~/static/icon.png"
        class="navbar-brand btn btn-link ml-2 nav-icon"
      />
      <a href="/" class="navbar-brand btn btn-link">Storm Net</a>
      <a href="/about" class="btn btn-link">About</a>
    </section>
  </header>
</template>

<script lang="ts">
import Vue from 'vue'
export default Vue.extend({
  name: 'Navbar'
})
</script>

<style lang="scss">
.nav-icon {
  max-width: 2.5rem;
}
</style>
