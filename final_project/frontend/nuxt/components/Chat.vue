<template>
  <div>
    <v-select
      :options="chatOptions"
      v-model="destination"
      @select="onChatSelect"
      placeholder="select chat"
      label="label"
    >
      <span slot="no-options">{{
        responder ? 'no one needs assistance' : 'no responders found'
      }}</span>
    </v-select>
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
      class="chat mt-2"
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
const personInNeedId = 3

for (let i = 0; i < chatConf.emergencyServicesAddresses.length; i++) {
  emergencyOptions.push({
    label: `emergency services ${i + 1}`,
    value: chatConf.emergencyServicesAddresses[i]
  })
}

const messageType = '\x00'
const connectType = '\x01'

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
          name: 'Person in Need',
          id: personInNeedId
        }
      ],
      me: {
        name: 'me',
        id: 1
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
            text: '#0a0a0a'
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
    this.$axios
      .get(chatConf.configEndpoint)
      .then((res) => {
        if (res.status === 200 && res.data.hasOwnProperty('radiosource')) {
          if (
            emergencyOptions.find(
              (option) =>
                option.value.charCodeAt(0) === parseInt(res.data.radiosource[0])
            )
          ) {
            this.responder = true
          }
          this.radiosource = res.data.radiosource
          this.socket = new window.WebSocket(
            `ws://${process.env.apiurl.replace('http://', '')}${
              chatConf.websocketEndpoint
            }`
          )
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
                    ? emergencyServicesId
                    : personInNeedId,
                  uploaded: false,
                  viewed: false
                }
                this.newMessage(message)
                setTimeout(() => {
                  this.scrollToBottom()
                }, 100)
              }
            } else if (jsonData.hasOwnProperty('connect')) {
              this.chatOptions.push({
                value: String.fromCharCode(jsonData.connect),
                label: `emergency ${this.chatOptions.length}`
              })
            } else if (jsonData.hasOwnProperty('currentid')) {
              this.websocketid = String.fromCharCode(jsonData.currentid)
              this.updateChatSelection()
            } else if (jsonData.hasOwnProperty('disconnect')) {
              const index = this.chatOptions.findIndex(
                (elem) =>
                  elem.value === String.fromCharCode(jsonData.disconnect)
              )
              if (index > -1) this.chatOptions.splice(index, 1)
            } else if (jsonData.hasOwnProperty('connectionOptions')) {
              const options = []
              for (let i = 0; i < jsonData.connectionOptions.length; i++) {
                options.push({
                  value: {
                    radio: String.fromCharCode(
                      jsonData.connectionOptions[i][0]
                    ),
                    id: String.fromCharCode(jsonData.connectionOptions[i][1])
                  },
                  label: `emergency ${i + 1}`
                })
              }
              this.chatOptions = options
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
          this.$toasted.global.info({ message: 'no source found' })
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
      this.messages = []
      if (!this.responder) {
        const theMessage =
          this.destination.value.radio +
          this.destination.value.id +
          connectType +
          ''
        this.socket.send(theMessage)
      }
    },
    updateChatSelection() {
      console.log('update chat selection')
      if (this.responder) {
        this.$axios
          .get(chatConf.alreadyConnectedEndpoint)
          .then((res) => {
            if (res.status === 200 && res.data.hasOwnProperty('connected')) {
              const senders = []
              for (let i = 0; i < res.data.connected.length; i++) {
                senders.push({
                  value: {
                    radio: String.fromCharCode(res.data.connected[i][0]),
                    id: String.fromCharCode(res.data.connected[i][1])
                  },
                  label: `emergency ${i + 1}`
                })
              }
              this.chatOptions = senders
            } else {
              this.$toasted.global.info({
                message: 'problem finding connections'
              })
            }
          })
          .catch((err) => {
            console.log(err)
            this.$toasted.global.error({
              message: `got error with get alreadyConnected request ${JSON.stringify(
                err
              )}`
            })
          })
      } else {
        this.$axios
          .put(chatConf.potentialConnectionsEndpoint, {
            radio: this.radiosource,
            websocketid: this.websocketid.charCodeAt(0)
          })
          .then((res) => {
            if (res.status === 200 && res.data.hasOwnProperty('message')) {
              console.log(res.data.message)
            } else {
              this.$toasted.global.info({ message: 'no success message found' })
            }
          })
          .catch((err) => {
            if (err.response) {
              console.log(err.response.data)
            }
            console.log(err)
            this.$toasted.global.error({
              message: `got error with get potential connections request ${JSON.stringify(
                err
              )}`
            })
          })
      }
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
          this.destination.value.radio +
          this.destination.value.id +
          messageType +
          message.content
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
  height: 40rem;
}
.chat .custom-title {
  color: #ffffff;
  font-size: 20px;
}
</style>
