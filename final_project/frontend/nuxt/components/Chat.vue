<template>
  <div>
    <p>{{ responder ? 'responder' : 'civilian' }}</p>
    <v-select
      v-if="responder"
      :options="chatOptions"
      v-model="destination"
      @input="onChatSelect"
      placeholder="select chat"
      label="label"
    >
      <span slot="no-options">
        {{ responder ? 'no one needs assistance' : 'no responders found' }}
      </span>
    </v-select>
    <table
      v-if="location.latitude && location.longitude"
      class="table table-striped table-hover"
    >
      <thead>
        <tr>
          <th>Name</th>
          <th>Value</th>
        </tr>
      </thead>
      <tbody>
        <tr
          @mouseover="hovering.latitude = true"
          @mouseout="hovering.latitude = false"
          :class="{ active: hovering.latitude }"
        >
          <td>latitude</td>
          <td>{{ location.latitude }}</td>
        </tr>
        <tr
          @mouseover="hovering.longitude = true"
          @mouseout="hovering.longitude = false"
          :class="{ active: hovering.longitude }"
        >
          <td>longitude</td>
          <td>{{ location.longitude }}</td>
        </tr>
      </tbody>
    </table>
    <Chat
      :participants="participants"
      :myself="me"
      :messages="messages"
      :onType="onType"
      :onMessageSubmit="onMessageSubmit"
      :chatTitle="'Emergency Services'"
      :placeholder="'send message'"
      :colors="colors"
      :borderStyle="borderStyle"
      :hideCloseButton="true"
      :closeButtonIconSize="'20px'"
      :submitIconSize="'30px'"
      :asyncMode="false"
      class="chat"
    />
  </div>
</template>

<script>
import { mapMutations } from 'vuex'
import VueScrollTo from 'vue-scrollto'
import Chat from '~/components/chat/Chat.vue'
import { chatConf } from '~/assets/config'

const emergencyOptions = []
const emergencyServicesId = 2
const civilianId = 3

for (let i = 0; i < chatConf.emergencyServicesAddresses.length; i++) {
  emergencyOptions.push({
    label: `emergency services ${i + 1}`,
    value: chatConf.emergencyServicesAddresses[i]
  })
}

const messageType = '\x00'

export default {
  components: {
    Chat
  },
  data() {
    return {
      participants: [
        {
          name: 'Emergency Services',
          id: emergencyServicesId
        },
        {
          name: 'Civilian',
          id: civilianId
        }
      ],
      me: {
        name: 'me',
        id: 1
      },
      hovering: {
        latitude: false,
        longitude: false
      },
      location: {
        latitude: null,
        longitude: null
      },
      socket: null,
      responder: false,
      radiosource: '',
      websocketid: '',
      destination: null,
      cancleScroll: null,
      chatOptions: [],
      messages: [],
      colors: {
        header: {
          bg: '#2143cc',
          text: '#ffffff'
        },
        message: {
          myself: {
            bg: '#fff',
            text: '#0a0a0a'
          },
          others: {
            bg: '#4061e6',
            text: '#fff'
          },
          messagesDisplay: {
            bg: '#f7f3f3'
          }
        },
        submitIcon: '#b91010'
      },
      borderStyle: {
        topLeft: '10px',
        topRight: '10px',
        bottomLeft: '10px',
        bottomRight: '10px'
      }
    }
  },
  computed: {
    myself() {
      return this.$store.state.chat.myself
    }
  },
  beforeDestroy() {
    if (this.socket) this.socket.close()
    this.messages = []
  },
  mounted() {
    this.getLocation()
    this.$axios
      .get(chatConf.configEndpoint)
      .then((res) => {
        if (res.status === 200 && res.data.hasOwnProperty('source')) {
          if (
            emergencyOptions.find(
              (option) =>
                option.value.charCodeAt(0) === parseInt(res.data.source)
            )
          ) {
            this.responder = true
            // the current node is the emergency worker
            this.updateChatSelection()
          } else {
            // the current node is a person who needs help
            this.destination = emergencyOptions[0]
          }
          this.socket = new window.WebSocket(
            `ws://${process.env.apiurl.replace('http://', '')}${
              chatConf.websocketEndpoint
            }`
          )
          this.radiosource = res.data.source
          this.socket.onopen = (evt) => {
            this.$toasted.global.success({
              message: 'Connection established'
            })
          }
          this.socket.onmessage = (evt) => {
            // console.log(`[message] Data received from server: ${evt.data}`)
            let jsonData
            try {
              jsonData = JSON.parse(evt.data)
            } catch (err) {
              this.$toasted.global.error({ message: JSON.stringify(err) })
            }
            if (jsonData.hasOwnProperty('debug')) {
              console.log(jsonData.debug)
            } else if (jsonData.hasOwnProperty('message')) {
              if (this.destination) {
                const message = {
                  content: jsonData.message,
                  myself: false,
                  participantId: this.responder
                    ? civilianId
                    : emergencyServicesId,
                  uploaded: false,
                  viewed: false
                }
                this.newMessage(message)
                setTimeout(() => {
                  this.scrollToBottom()
                }, 100)
              }
            } else if (jsonData.hasOwnProperty('location')) {
              console.log('received location!')
              if (this.responder) {
                const location = jsonData.location.split(',')
                this.location = {
                  latitude: location[0],
                  longitude: location[1]
                }
              } else {
                this.getLocation()
              }
            }
          }
          this.socket.onclose = (evt) => {
            if (evt.wasClean) {
              console.log(
                `[close] Connection closed cleanly, code=${evt.code} reason=${evt.reason}`
              )
            } else {
              this.$toasted.global.error({ message: '[close] Connection died' })
            }
          }
          this.socket.onerror = (err) => {
            console.error(`[error]: `, err)
          }
        } else {
          this.$toasted.global.warn({ message: 'no source found' })
        }
      })
      .catch((err) => {
        const message = 'got error with get config request'
        this.$toasted.global.error({ message })
        console.log(message, err)
      })
  },
  methods: {
    ...mapMutations({ newMessage: 'chat/newMessage' }),
    onChatSelect() {
      console.log('selected location')
      this.messages = []
      this.location = {
        latitude: null,
        longitude: null
      }
      this.requestLocation()
    },
    requestLocation() {
      console.log('requesting location')
      console.log(`got destination: ${this.destination.value.charCodeAt(0)}`)
      this.$axios
        .put(chatConf.locationEndpoint, {
          destination: this.destination.value.charCodeAt(0)
        })
        .then((res) => {
          if (res.status === 200) {
            if (res.data.hasOwnProperty('message')) {
              this.$toasted.global.success({ message: res.data.message })
            } else {
              this.$toasted.global.success({
                message: 'success requesting location'
              })
            }
          } else {
            this.$$toasted.global.error({
              message: 'did not get 200 for location request'
            })
          }
        })
        .catch((err) => {
          const message = `problem requesting location: ${JSON.stringify(
            err.response.data
          )}`
          this.$toasted.global.error({ message })
          console.log(message, err)
        })
    },
    getLocation() {
      if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(
          (pos) => {
            console.log(pos)
            this.location.latitude = pos.coords.latitude
            this.location.longitude = pos.coords.longitude
            console.log('got location')
            console.log(this.location)
            this.sendLocation()
          },
          (err) => {
            console.log(err.message)
          }
        )
        console.log('done')
      } else {
        console.error('this browser does not support geolocation')
      }
    },
    sendLocation() {
      console.log('sending location')
      this.$axios
        .put(chatConf.locationEndpoint, {
          latitude: this.location.latitude,
          longitude: this.location.longitude,
          destination: this.destination.value
        })
        .then((res) => {
          if (res.status === 200) {
            if (res.data.hasOwnProperty('message')) {
              this.$toasted.global.success({ message: res.data.message })
            } else {
              this.$toasted.global.success({
                message: 'success sending location'
              })
            }
          } else {
            this.$$toasted.global.error({
              message: 'did not get 200 for location send'
            })
          }
        })
        .catch((err) => {
          const message = `got error with location send request: ${JSON.stringify(
            err
          )}`
          this.$toasted.global.error({ message })
          console.log(message, err)
        })
    },
    updateChatSelection() {
      this.$axios
        .get(chatConf.senderListEndpoint)
        .then((res) => {
          if (res.status === 200 && res.data.hasOwnProperty('senders')) {
            const senders = []
            for (let i = 0; i < res.data.senders.length; i++) {
              senders.push({
                value: String.fromCharCode(res.data.senders[i]),
                label: `emergency ${i + 1}`
              })
            }
            this.chatOptions = senders
          } else {
            this.$toasted.global.warn({ message: 'no senders found' })
          }
        })
        .catch((err) => {
          this.$toasted.global.error({
            message: `got error with get senders request ${JSON.stringify(err)}`
          })
        })
    },
    onType(event) {
      // here you can set any behavior
    },
    scrollToBottom() {
      if (this.cancleScroll) {
        this.cancleScroll()
        this.cancleScroll = null
      }
      this.cancleScroll = VueScrollTo.scrollTo(
        `#message-${this.$store.state.chat.messages.length - 1}`,
        {
          container: '#containerMessageDisplay',
          offset: 0,
          force: true,
          cancelable: true
        }
      )
    },
    onMessageSubmit(message) {
      console.log(`New message sending: ${JSON.stringify(message)}`)
      if (this.destination) {
        const theMessage =
          this.destination.value + messageType + message.content
        this.socket.send(theMessage)
        message.uploaded = true
      } else {
        this.$toasted.global.error({ message: 'no destination found' })
      }
      setTimeout(() => {
        this.scrollToBottom()
      }, 100)
    }
  }
}
</script>

<style lang="scss" scoped>
.chat {
  max-width: 30rem;
  margin: auto;
  height: 40rem;
  @media only screen and (min-width: 600px) {
    margin-top: 2rem;
  }
}
.chat .custom-title {
  color: #ffffff;
  font-size: 20px;
}
</style>
