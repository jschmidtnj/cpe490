<template>
  <div class="hero bg-light">
    <div class="hero-body">
      <h1 class="about-header">About</h1>
      <ul class="about-list">
        <li>mesh network for emergency response.</li>
        <li>connect and communicate with a responder.</li>
      </ul>
    </div>
  </div>
</template>

<script lang="ts">
import Vue from 'vue'
export default Vue.extend({
  name: 'About'
})
</script>

<style lang="scss">
.about-header {
  margin-bottom: 5rem;
  @media only screen and (max-width: 500px) {
    text-align: center;
  }
  @media only screen and (min-width: 500px) {
    margin-left: 15%;
  }
}
.about-list {
  @media only screen and (min-width: 500px) {
    margin-left: 15%;
    margin-top: 5rem;
  }
}
</style>
